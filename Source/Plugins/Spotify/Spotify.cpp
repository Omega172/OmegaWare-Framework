#include "OmegaWarePlugin.hpp"

#include "Artwork.hpp"
#include "Http.hpp"
#include "Json.hpp"
#include "Media.hpp"

#include <Windows.h>
#include <shellapi.h>
#include <wincrypt.h>

#include <winrt/base.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <format>
#include <memory>
#include <mutex>
#include <optional>
#include <stop_token>
#include <string>
#include <thread>
#include <vector>

#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "shell32.lib")

namespace
{
	using namespace OmegaWare;
	using namespace std::chrono_literals;
	using Clock = std::chrono::steady_clock;

	constexpr const char* kKeyConfig = "spotify.key";
	constexpr const char* kIntervalConfig = "spotify.interval";
	constexpr const char* kWindowConfig = "spotify.window";
	constexpr const char* kPinnedConfig = "spotify.window_pinned";
	constexpr const char* kOpacityConfig = "spotify.window_opacity";
	constexpr std::string_view kProtectedPrefix = "dpapi:";
	constexpr std::size_t kMinKeyLength = 120;
	constexpr int kArtSize = 256;
	constexpr float kArtDisplay = 96.f;
	constexpr wchar_t kRefreshUrl[] = L"https://spotify.kahool.dev/api/refresh?refresh_token=";
	constexpr wchar_t kPlayingUrl[] = L"https://api.spotify.com/v1/me/player/currently-playing?additional_types=episode";
	constexpr wchar_t kKeySite[] = L"https://spotify.kahool.dev/";

	[[nodiscard]] std::string Protect(std::string_view strPlain)
	{
		DATA_BLOB stInput{ static_cast<DWORD>(strPlain.size()), reinterpret_cast<BYTE*>(const_cast<char*>(strPlain.data())) };
		DATA_BLOB stOutput{};
		if (!CryptProtectData(&stInput, L"OmegaWare Spotify key", nullptr, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &stOutput))
			return {};
		DWORD uLength = 0;
		std::string strEncoded;
		if (CryptBinaryToStringA(stOutput.pbData, stOutput.cbData, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, nullptr, &uLength))
		{
			strEncoded.resize(uLength);
			if (CryptBinaryToStringA(stOutput.pbData, stOutput.cbData, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, strEncoded.data(), &uLength))
				strEncoded.resize(uLength);
			else
				strEncoded.clear();
		}
		LocalFree(stOutput.pbData);
		return strEncoded.empty() ? std::string{} : std::string(kProtectedPrefix) + strEncoded;
	}

	[[nodiscard]] std::string Unprotect(std::string_view strStored)
	{
		if (!strStored.starts_with(kProtectedPrefix))
			return {};
		strStored.remove_prefix(kProtectedPrefix.size());
		DWORD uLength = 0;
		if (!CryptStringToBinaryA(strStored.data(), static_cast<DWORD>(strStored.size()), CRYPT_STRING_BASE64, nullptr, &uLength, nullptr, nullptr))
			return {};
		std::vector<BYTE> aBlob(uLength);
		if (!CryptStringToBinaryA(strStored.data(), static_cast<DWORD>(strStored.size()), CRYPT_STRING_BASE64, aBlob.data(), &uLength, nullptr, nullptr))
			return {};
		DATA_BLOB stInput{ uLength, aBlob.data() };
		DATA_BLOB stOutput{};
		if (!CryptUnprotectData(&stInput, nullptr, nullptr, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &stOutput))
			return {};
		std::string strPlain(reinterpret_cast<const char*>(stOutput.pbData), stOutput.cbData);
		SecureZeroMemory(stOutput.pbData, stOutput.cbData);
		LocalFree(stOutput.pbData);
		return strPlain;
	}

	[[nodiscard]] std::string Trim(std::string_view strText)
	{
		const std::size_t uBegin = strText.find_first_not_of(" \t\r\n\"'");
		if (uBegin == std::string_view::npos)
			return {};
		const std::size_t uEnd = strText.find_last_not_of(" \t\r\n\"'");
		return std::string(strText.substr(uBegin, uEnd - uBegin + 1));
	}

	[[nodiscard]] float TextWidth(const std::string& strText)
	{
		float flWidth = 0.f;
		Plugin::Api().CalcTextSize(strText.c_str(), &flWidth, nullptr);
		return flWidth;
	}

	[[nodiscard]] std::string FitText(const std::string& strText, float flWidth)
	{
		if (TextWidth(strText) <= flWidth)
			return strText;
		std::size_t uLow = 0;
		std::size_t uHigh = strText.size();
		while (uLow < uHigh)
		{
			std::size_t uMid = (uLow + uHigh + 1) / 2;
			while (uMid > 0 && (static_cast<unsigned char>(strText[uMid]) & 0xC0) == 0x80)
				--uMid;
			if (uMid <= uLow)
				break;
			if (TextWidth(strText.substr(0, uMid) + "...") <= flWidth)
				uLow = uMid;
			else
				uHigh = uMid - 1;
		}
		while (uLow > 0 && (static_cast<unsigned char>(strText[uLow]) & 0xC0) == 0x80)
			--uLow;
		std::string strFit = strText.substr(0, uLow);
		while (!strFit.empty() && strFit.back() == ' ')
			strFit.pop_back();
		return strFit + "...";
	}

	[[nodiscard]] std::string FormatTime(int iMilliseconds)
	{
		const int iSeconds = std::max(0, iMilliseconds / 1000);
		return std::format("{}:{:02}", iSeconds / 60, iSeconds % 60);
	}

	struct stTrack
	{
		bool bValid = false;
		std::string strId;
		std::string strTitle;
		std::string strArtists;
		std::string strAlbum;
		std::string strArtUrl;
		int iProgressMs = 0;
		int iDurationMs = 0;
		bool bPlaying = false;
		Clock::time_point tpFetched{};

		[[nodiscard]] int ProgressNow() const
		{
			if (!bPlaying)
				return iProgressMs;
			const auto iElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - tpFetched).count();
			return static_cast<int>(std::min<long long>(iDurationMs, iProgressMs + iElapsed));
		}
	};

	struct stSnapshot
	{
		stTrack stCurrent;
		std::string strStatus;
		bool bHasKey = false;
	};

	class clClient
	{
	public:
		void Start()
		{
			if (m_thWorker.joinable())
				return;
			m_pHttp = std::make_unique<Spotify::Http::clClient>();
			m_thWorker = std::jthread([this](std::stop_token stToken) { Worker(stToken); });
		}

		void Stop()
		{
			if (!m_thWorker.joinable())
				return;
			m_thWorker.request_stop();
			m_pHttp->Cancel();
			m_thWorker.join();
			m_pHttp.reset();
		}

		void SetKey(std::string strKey)
		{
			{
				std::scoped_lock lckState(m_mtxState);
				if (!m_strKey.empty())
					SecureZeroMemory(m_strKey.data(), m_strKey.size());
				m_strKey = std::move(strKey);
				m_bKeyChanged = true;
				m_bWake = true;
			}
			m_cvWake.notify_all();
		}

		void Command(Spotify::Media::ECommand eCommand)
		{
			{
				std::scoped_lock lckState(m_mtxState);
				m_aCommands.push_back(eCommand);
				m_bWake = true;
			}
			m_cvWake.notify_all();
		}

		void MarkViewed(float flInterval)
		{
			bool bNotify = false;
			{
				std::scoped_lock lckState(m_mtxState);
				const Clock::time_point tpNow = Clock::now();
				bNotify = tpNow - m_tpViewed > 3s;
				m_tpViewed = tpNow;
				m_flInterval = std::clamp(flInterval, 1.f, 30.f);
				if (bNotify)
					m_bWake = true;
			}
			if (bNotify)
				m_cvWake.notify_all();
		}

		[[nodiscard]] stSnapshot Snapshot()
		{
			std::scoped_lock lckState(m_mtxState);
			return { m_stTrack, m_strStatus, !m_strKey.empty() };
		}

		[[nodiscard]] std::optional<Spotify::Artwork::stImage> TakeArtwork()
		{
			std::scoped_lock lckState(m_mtxState);
			return std::exchange(m_optArtwork, std::nullopt);
		}

		[[nodiscard]] std::optional<std::string> TakeRotatedKey()
		{
			std::scoped_lock lckState(m_mtxState);
			return std::exchange(m_optRotatedKey, std::nullopt);
		}

	private:
		void SetStatus(std::string strStatus)
		{
			std::scoped_lock lckState(m_mtxState);
			m_strStatus = std::move(strStatus);
		}

		void ClearTrack(std::string strStatus)
		{
			m_strLoadedArt.clear();
			std::scoped_lock lckState(m_mtxState);
			m_stTrack = {};
			m_strStatus = std::move(strStatus);
		}

		[[nodiscard]] static std::string ErrorMessage(const Spotify::Http::stResponse& rResponse)
		{
			const std::optional<Spotify::Json::stValue> optRoot = Spotify::Json::Parse(rResponse.strBody);
			if (optRoot)
			{
				const Spotify::Json::stValue& rError = (*optRoot)["error"];
				if (!rError["message"].String().empty())
					return std::string(rError["message"].String());
				if (!rError.String().empty())
					return std::string(rError.String());
			}
			return {};
		}

		bool Refresh(const std::string& strKey, std::string& rAccess, Clock::time_point& rExpiry, Clock::time_point& rNextPoll)
		{
			const Spotify::Http::stResponse stResponse = m_pHttp->Get(std::wstring(kRefreshUrl) + Spotify::Http::Widen(Spotify::Http::UrlEncode(strKey)), {}, 64 * 1024);
			if (stResponse.uError && !stResponse.uStatus)
			{
				SetStatus("could not reach spotify.kahool.dev, retrying");
				rNextPoll = Clock::now() + 10s;
				return false;
			}
			if (stResponse.uStatus == 200)
			{
				const std::optional<Spotify::Json::stValue> optRoot = Spotify::Json::Parse(stResponse.strBody);
				const std::string_view strAccess = optRoot ? (*optRoot)["access_token"].String() : std::string_view{};
				if (!strAccess.empty())
				{
					rAccess = std::string(strAccess);
					const double dExpires = std::clamp((*optRoot)["expires_in"].Number(3600.0), 120.0, 86400.0);
					rExpiry = Clock::now() + std::chrono::seconds(static_cast<long long>(dExpires) - 60);
					const std::string_view strRotated = (*optRoot)["refresh_token"].String();
					if (!strRotated.empty() && strRotated != strKey)
					{
						std::scoped_lock lckState(m_mtxState);
						m_strKey = std::string(strRotated);
						m_optRotatedKey = m_strKey;
					}
					return true;
				}
			}
			const std::string strError = ErrorMessage(stResponse);
			std::string strLower = strError;
			for (char& rChar : strLower)
				rChar = static_cast<char>(std::tolower(static_cast<unsigned char>(rChar)));
			if (stResponse.uStatus == 400 || stResponse.uStatus == 401 || (stResponse.uStatus == 500 && strLower.find("invalid") != std::string::npos))
			{
				{
					std::scoped_lock lckState(m_mtxState);
					m_bKeyRejected = true;
				}
				ClearTrack("the key was rejected, generate a new one at spotify.kahool.dev");
				Plugin::Log(Plugin::ELogLevel::Warning, std::format("spotify.kahool.dev rejected the stored key (HTTP {}{}{})", stResponse.uStatus, strError.empty() ? "" : ": ", strError));
				return false;
			}
			SetStatus(std::format("spotify.kahool.dev returned HTTP {}{}{}, retrying", stResponse.uStatus, strError.empty() ? "" : ": ", strError));
			rNextPoll = Clock::now() + 30s;
			return false;
		}

		[[nodiscard]] static std::string PickArtUrl(const Spotify::Json::stValue& rImages)
		{
			std::string strBest;
			double dBestWidth = 0.0;
			for (const Spotify::Json::stValue& rImage : rImages.aItems)
			{
				const std::string_view strUrl = rImage["url"].String();
				if (strUrl.empty())
					continue;
				const double dWidth = rImage["width"].Number(0.0);
				const bool bBestTooSmall = dBestWidth < kArtSize;
				if (strBest.empty() || (bBestTooSmall && dWidth > dBestWidth) || (!bBestTooSmall && dWidth >= kArtSize && dWidth < dBestWidth))
				{
					strBest = std::string(strUrl);
					dBestWidth = dWidth;
				}
			}
			return strBest;
		}

		void UpdateArtwork(const std::string& strUrl)
		{
			if (strUrl.empty() || strUrl == m_strLoadedArt)
				return;
			m_strLoadedArt = strUrl;
			const Spotify::Http::stResponse stResponse = m_pHttp->Get(Spotify::Http::Widen(strUrl));
			if (stResponse.uStatus != 200 || stResponse.strBody.empty())
				return;
			std::optional<Spotify::Artwork::stImage> optImage = Spotify::Artwork::Decode({ reinterpret_cast<const std::uint8_t*>(stResponse.strBody.data()), stResponse.strBody.size() }, kArtSize);
			if (!optImage)
				return;
			std::scoped_lock lckState(m_mtxState);
			m_optArtwork = std::move(optImage);
		}

		void ApplyPlaying(const Spotify::Json::stValue& rRoot)
		{
			const Spotify::Json::stValue& rItem = rRoot["item"];
			if (rItem.IsNull())
			{
				ClearTrack(rRoot["currently_playing_type"].String() == "ad" ? "an ad is playing" : "nothing to show for what is playing");
				return;
			}
			stTrack stNew;
			stNew.bValid = true;
			stNew.strId = std::string(rItem["id"].String());
			stNew.strTitle = std::string(rItem["name"].String());
			const bool bEpisode = rItem["type"].String() == "episode";
			if (bEpisode)
			{
				stNew.strArtists = std::string(rItem["show"]["publisher"].String());
				stNew.strAlbum = std::string(rItem["show"]["name"].String());
			}
			else
			{
				for (const Spotify::Json::stValue& rArtist : rItem["artists"].aItems)
				{
					if (!stNew.strArtists.empty())
						stNew.strArtists += ", ";
					stNew.strArtists += rArtist["name"].String();
				}
				stNew.strAlbum = std::string(rItem["album"]["name"].String());
			}
			const Spotify::Json::stValue& rImages = bEpisode ? (rItem["images"].Size() ? rItem["images"] : rItem["show"]["images"]) : rItem["album"]["images"];
			stNew.strArtUrl = PickArtUrl(rImages);
			stNew.iDurationMs = static_cast<int>(rItem["duration_ms"].Number(0.0));
			stNew.iProgressMs = static_cast<int>(rRoot["progress_ms"].Number(0.0));
			stNew.bPlaying = rRoot["is_playing"].Bool();
			stNew.tpFetched = Clock::now();
			{
				std::scoped_lock lckState(m_mtxState);
				m_stTrack = stNew;
				m_strStatus.clear();
			}
			UpdateArtwork(stNew.strArtUrl);
		}

		void RunCommands(Spotify::Media::clController& rMedia, const std::vector<Spotify::Media::ECommand>& aCommands, Clock::time_point& rNextPoll)
		{
			for (const Spotify::Media::ECommand eCommand : aCommands)
			{
				const Spotify::Media::EResult eResult = rMedia.Send(eCommand);
				if (eResult == Spotify::Media::EResult::NoSession)
				{
					SetStatus("playback controls need the Spotify app running on this PC");
					continue;
				}
				if (eResult == Spotify::Media::EResult::Failed)
				{
					SetStatus("Spotify did not accept the command");
					continue;
				}
				std::scoped_lock lckState(m_mtxState);
				if (eCommand == Spotify::Media::ECommand::PlayPause && m_stTrack.bValid)
				{
					m_stTrack.iProgressMs = m_stTrack.ProgressNow();
					m_stTrack.tpFetched = Clock::now();
					m_stTrack.bPlaying = !m_stTrack.bPlaying;
				}
				rNextPoll = std::min(rNextPoll, Clock::now() + 600ms);
			}
		}

		void Worker(std::stop_token stToken)
		{
			bool bApartment = false;
			try
			{
				winrt::init_apartment(winrt::apartment_type::multi_threaded);
				bApartment = true;
			}
			catch (const winrt::hresult_error&)
			{
			}
			{
				Spotify::Media::clController clMedia;
				std::string strAccess;
				Clock::time_point tpExpiry{};
				Clock::time_point tpNextPoll = Clock::now();
				bool bRetriedAuth = false;
				bool bIdle = false;
				while (!stToken.stop_requested())
				{
					std::vector<Spotify::Media::ECommand> aCommands;
					std::string strKey;
					bool bKeyChanged = false;
					bool bViewed = false;
					bool bRejected = false;
					float flInterval = 2.f;
					{
						std::unique_lock lckState(m_mtxState);
						m_cvWake.wait_until(lckState, stToken, bIdle ? Clock::now() + 24h : tpNextPoll, [this] { return m_bWake; });
						if (stToken.stop_requested())
							break;
						m_bWake = false;
						aCommands = std::exchange(m_aCommands, {});
						bKeyChanged = std::exchange(m_bKeyChanged, false);
						if (bKeyChanged)
							m_bKeyRejected = false;
						strKey = m_strKey;
						bRejected = m_bKeyRejected;
						bViewed = Clock::now() - m_tpViewed < 3s;
						flInterval = m_flInterval;
					}

					if (bKeyChanged)
					{
						strAccess.clear();
							tpNextPoll = Clock::now();
						if (strKey.empty())
							ClearTrack({});
						else
							ClearTrack("connecting");
					}
					if (!aCommands.empty())
						RunCommands(clMedia, aCommands, tpNextPoll);
					bIdle = strKey.empty() || bRejected || !bViewed;
					if (bIdle)
						continue;
					const Clock::time_point tpNow = Clock::now();
					if (tpNow < tpNextPoll)
						continue;
					tpNextPoll = tpNow + std::chrono::milliseconds(static_cast<long long>(flInterval * 1000.f));

					if (strAccess.empty() || tpNow >= tpExpiry)
					{
						strAccess.clear();
						if (!Refresh(strKey, strAccess, tpExpiry, tpNextPoll))
							continue;
					}

					const Spotify::Http::stResponse stResponse = m_pHttp->Get(kPlayingUrl, Spotify::Http::Widen(std::format("Authorization: Bearer {}\r\nAccept: application/json", strAccess)), 1024 * 1024);
					if (stToken.stop_requested())
						break;
					if (stResponse.uError && !stResponse.uStatus)
					{
						SetStatus("could not reach Spotify, retrying");
						tpNextPoll = Clock::now() + 5s;
						continue;
					}
					if (stResponse.uStatus == 401 && !bRetriedAuth)
					{
						bRetriedAuth = true;
						strAccess.clear();
						tpNextPoll = Clock::now();
						continue;
					}
					if (stResponse.uStatus != 401)
						bRetriedAuth = false;
					if (stResponse.uStatus == 200)
					{
						const std::optional<Spotify::Json::stValue> optRoot = Spotify::Json::Parse(stResponse.strBody);
						if (optRoot)
							ApplyPlaying(*optRoot);
						else
							SetStatus("Spotify sent a response that could not be read");
					}
					else if (stResponse.uStatus == 204)
						ClearTrack("nothing is playing");
					else if (stResponse.uStatus == 429)
					{
						const int iWait = std::clamp(stResponse.iRetryAfter > 0 ? stResponse.iRetryAfter : 5, 1, 600);
						SetStatus(std::format("Spotify is rate limiting, retrying in {} s", iWait));
						tpNextPoll = Clock::now() + std::chrono::seconds(iWait);
					}
					else
					{
						const std::string strError = ErrorMessage(stResponse);
						SetStatus(std::format("Spotify returned HTTP {}{}{}", stResponse.uStatus, strError.empty() ? "" : ": ", strError));
						tpNextPoll = Clock::now() + std::max(10s, std::chrono::seconds(static_cast<long long>(flInterval)));
					}
				}
				if (!strAccess.empty())
					SecureZeroMemory(strAccess.data(), strAccess.size());
			}
			if (bApartment)
				winrt::uninit_apartment();
		}

		std::unique_ptr<Spotify::Http::clClient> m_pHttp;
		std::string m_strLoadedArt;
		std::mutex m_mtxState;
		std::condition_variable_any m_cvWake;
		bool m_bWake = false;
		std::string m_strKey;
		bool m_bKeyChanged = false;
		bool m_bKeyRejected = false;
		std::vector<Spotify::Media::ECommand> m_aCommands;
		Clock::time_point m_tpViewed{};
		float m_flInterval = 2.f;
		stTrack m_stTrack;
		std::string m_strStatus;
		std::optional<Spotify::Artwork::stImage> m_optArtwork;
		std::optional<std::string> m_optRotatedKey;
		std::jthread m_thWorker;
	};

	class clSpotifyFeature final : public Plugin::clFeature
	{
	public:
		[[nodiscard]] const char* GetName() const override { return "spotify"; }

		bool Setup() override
		{
			m_pflInterval = Plugin::Api().ConfigFloat(kIntervalConfig, 2.f);
			m_pbWindow = Plugin::Api().ConfigBool(kWindowConfig, true);
			m_pbPinned = Plugin::Api().ConfigBool(kPinnedConfig, false);
			m_pflOpacity = Plugin::Api().ConfigFloat(kOpacityConfig, 100.f);
			LoadStoredKey();
			m_clClient.Start();
			return true;
		}

		void Destroy() override
		{
			m_clClient.Stop();
			ClearArt();
			SecureZeroMemory(m_aKeyInput.data(), m_aKeyInput.size());
		}

		void Render() override
		{
			Update();
			DrawWindow();
		}

		void DrawMenu() override
		{
			m_clClient.MarkViewed(*m_pflInterval);
			if (!Plugin::Api().BeginSection("spotify"))
				return;
			if (!m_stState.bHasKey)
				DrawKeyEntry();
			else
				DrawPlayer();
			Plugin::Api().EndSection();
			if (Plugin::Api().BeginSection("window"))
			{
				Plugin::Api().Toggle("player window", m_pbWindow);
				Plugin::Api().Checkbox("always visible", m_pbPinned);
				Plugin::Api().SliderFloat("background opacity", m_pflOpacity, 0.f, 100.f, "{:.0f}%");
				Plugin::Api().EndSection();
			}
		}

	private:
		void Update()
		{
			LoadStoredKey();
			if (std::optional<std::string> optRotated = m_clClient.TakeRotatedKey())
			{
				StoreKey(*optRotated);
				SecureZeroMemory(optRotated->data(), optRotated->size());
			}
			if (std::optional<Spotify::Artwork::stImage> optArt = m_clClient.TakeArtwork())
			{
				ClearArt();
				m_pArt = Plugin::Api().ImageCreate(Plugin::Api().pHost, optArt->aPixels.data(), optArt->iWidth, optArt->iHeight);
				m_uPrimary = optArt->uPrimary;
			}

			m_stState = m_clClient.Snapshot();
			if (!m_stState.stCurrent.bValid)
				ClearArt();
		}

		void DrawWindow()
		{
			if (!*m_pbWindow)
				return;
			float flDisplayWidth = 0.f;
			float flDisplayHeight = 0.f;
			Plugin::Api().GetDisplaySize(&flDisplayWidth, &flDisplayHeight);
			float flSpacingX = 0.f;
			Plugin::Api().GetItemSpacing(&flSpacingX, nullptr);
			float flLine = 0.f;
			Plugin::Api().CalcTextSize("Ag", nullptr, &flLine);
			const float flMinArt = std::max(40.f, flLine * 2.5f);
			if (m_flOverheadX > 0.f)
				Plugin::Api().SetNextWindowSizeConstraints(m_flOverheadX + flMinArt + flSpacingX + flLine * 5.f, m_flOverheadY + flMinArt, std::max(flDisplayWidth, 200.f), std::max(flDisplayHeight, 200.f));
			Plugin::Api().SetNextWindowPos(flDisplayWidth - 460.f, 60.f, false);
			Plugin::Api().SetNextWindowSize(420.f, 150.f, false);
			Plugin::Api().SetNextWindowBgAlpha(std::clamp(*m_pflOpacity, 0.f, 100.f) / 100.f);
			const Plugin::EWindowFlags eFlags = Plugin::EWindowFlags::NoScrollbar | Plugin::EWindowFlags::NoScrollWithMouse | (*m_pbPinned ? Plugin::EWindowFlags::AlwaysVisible : Plugin::EWindowFlags::None);
			if (Plugin::Api().BeginWindow("spotify###spotify.player", m_pbWindow, eFlags))
			{
				m_clClient.MarkViewed(*m_pflInterval);
				float flWindowWidth = 0.f;
				float flWindowHeight = 0.f;
				float flAvailWidth = 0.f;
				float flAvailHeight = 0.f;
				Plugin::Api().GetWindowSize(&flWindowWidth, &flWindowHeight);
				Plugin::Api().GetContentRegionAvail(&flAvailWidth, &flAvailHeight);
				m_flOverheadX = std::max(0.f, flWindowWidth - flAvailWidth);
				m_flOverheadY = std::max(0.f, flWindowHeight - flAvailHeight);
				if (!m_stState.bHasKey)
					Plugin::Api().TextMuted(FitText("paste a key on the spotify page", flAvailWidth).c_str());
				else if (!m_stState.stCurrent.bValid)
					Plugin::Api().TextMuted(FitText(m_stState.strStatus.empty() ? "waiting for Spotify" : m_stState.strStatus, flAvailWidth).c_str());
				else
					DrawScaledCard(std::max(flAvailWidth, 1.f), std::max(flAvailHeight, 1.f), flMinArt);
			}
			Plugin::Api().EndWindow();
		}

		enum class ERow
		{
			Title,
			Artists,
			Progress,
			Album,
			State,
		};

		void DrawScaledCard(float flWidth, float flHeight, float flMinArt)
		{
			constexpr std::array<ERow, 5> aOrder = { ERow::Title, ERow::Artists, ERow::Progress, ERow::Album, ERow::State };
			const stTrack& rTrack = m_stState.stCurrent;
			float flSpacingX = 0.f;
			float flSpacingY = 0.f;
			Plugin::Api().GetItemSpacing(&flSpacingX, &flSpacingY);
			float flLine = 0.f;
			Plugin::Api().CalcTextSize("Ag", nullptr, &flLine);
			const float flFrame = Plugin::Api().GetFrameHeight();
			const auto fnRowHeight = [&](ERow eRow) { return eRow == ERow::Progress ? flFrame : flLine; };
			const auto fnRowsHeight = [&](std::size_t uCount) {
				float flTotal = 0.f;
				for (std::size_t i = 0; i < uCount; ++i)
					flTotal += fnRowHeight(aOrder[i]) + (i ? flSpacingY : 0.f);
				return flTotal;
			};

			const bool bPortrait = flHeight > flWidth * 1.25f;
			std::size_t uRows = 1;
			float flArt = 0.f;
			float flRowWidth = 0.f;
			if (bPortrait)
			{
				const float flShortest = std::min(flWidth, flHeight);
				for (uRows = aOrder.size(); uRows > 1; --uRows)
					if (std::min(flWidth, flHeight - fnRowsHeight(uRows) - flSpacingY) >= flShortest * .55f)
						break;
				flArt = std::max(flMinArt, std::min(flWidth, flHeight - fnRowsHeight(uRows) - flSpacingY));
				flRowWidth = flWidth;
			}
			else
			{
				flArt = std::clamp(flHeight, flMinArt, std::max(flMinArt, flWidth - flSpacingX - flLine * 5.f));
				while (uRows < aOrder.size() && fnRowsHeight(uRows + 1) <= flArt)
					++uRows;
				flRowWidth = std::max(1.f, flWidth - flArt - flSpacingX);
			}

			if (bPortrait && flWidth > flArt + flSpacingX * 2.f)
			{
				Plugin::Api().Dummy((flWidth - flArt) * .5f - flSpacingX, 1.f);
				Plugin::Api().SameLine();
			}
			Plugin::Api().Image(m_pArt, flArt, flArt, Plugin::Rgba(255, 255, 255));
			if (!bPortrait)
			{
				Plugin::Api().SameLine();
				Plugin::Api().BeginGroup();
				const float flPad = (flArt - fnRowsHeight(uRows)) * .5f - flSpacingY;
				if (flPad > 0.f)
					Plugin::Api().Dummy(1.f, flPad);
			}

			const std::uint32_t uAccent = m_pArt ? m_uPrimary : Plugin::Api().AccentColor();
			for (std::size_t i = 0; i < uRows; ++i)
			{
				switch (aOrder[i])
				{
				case ERow::Title: Plugin::Api().TextColored(uAccent, FitText(rTrack.strTitle, flRowWidth).c_str()); break;
				case ERow::Artists: Plugin::Api().Text(FitText(rTrack.strArtists, flRowWidth).c_str()); break;
				case ERow::Album: Plugin::Api().TextMuted(FitText(rTrack.strAlbum, flRowWidth).c_str()); break;
				case ERow::State: Plugin::Api().TextMuted(rTrack.bPlaying ? "playing" : "paused"); break;
				case ERow::Progress:
				{
					const int iProgress = rTrack.ProgressNow();
					const float flFraction = rTrack.iDurationMs > 0 ? static_cast<float>(iProgress) / static_cast<float>(rTrack.iDurationMs) : 0.f;
					std::string strTime = std::format("{} / {}", FormatTime(iProgress), FormatTime(rTrack.iDurationMs));
					if (TextWidth(strTime) + flLine > flRowWidth)
						strTime = TextWidth(FormatTime(iProgress)) + flLine > flRowWidth ? std::string(" ") : FormatTime(iProgress);
					Plugin::Api().ProgressBar(flFraction, strTime.c_str(), uAccent);
					break;
				}
				}
			}
			if (!bPortrait)
				Plugin::Api().EndGroup();
		}

		void LoadStoredKey()
		{
			const char* szStored = Plugin::Api().ConfigString(kKeyConfig, "");
			const std::string_view strStored = szStored ? szStored : "";
			if (m_bKeyLoaded && strStored == m_strStoredKey)
				return;
			m_bKeyLoaded = true;
			m_strStoredKey = std::string(strStored);
			std::string strKey = Unprotect(strStored);
			if (!strStored.empty() && strKey.empty())
				Plugin::Log(Plugin::ELogLevel::Warning, "the stored Spotify key could not be decrypted for this Windows user, paste it again");
			m_clClient.SetKey(std::move(strKey));
		}

		void StoreKey(const std::string& strKey)
		{
			m_strStoredKey = strKey.empty() ? std::string{} : Protect(strKey);
			Plugin::Api().SetConfigString(kKeyConfig, m_strStoredKey.c_str());
			m_bKeyLoaded = true;
		}

		void ClearArt()
		{
			if (m_pArt)
				Plugin::Api().ImageDestroy(Plugin::Api().pHost, m_pArt);
			m_pArt = nullptr;
		}

		void DrawKeyEntry()
		{
			Plugin::Api().TextMuted("log in at spotify.kahool.dev, copy the code it shows and paste it here");
			Plugin::Api().InputText("key", m_aKeyInput.data(), static_cast<std::uint32_t>(m_aKeyInput.size()));
			if (Plugin::Api().Button("save key"))
			{
				std::string strKey = Trim(m_aKeyInput.data());
				SecureZeroMemory(m_aKeyInput.data(), m_aKeyInput.size());
				if (strKey.size() < kMinKeyLength)
					m_strEntryError = "that does not look like a key from spotify.kahool.dev";
				else
				{
					const std::string strProtected = Protect(strKey);
					if (strProtected.empty())
						m_strEntryError = "the key could not be encrypted";
					else
					{
						m_strEntryError.clear();
						m_strStoredKey = strProtected;
						Plugin::Api().SetConfigString(kKeyConfig, m_strStoredKey.c_str());
						m_clClient.SetKey(strKey);
						Plugin::Log(Plugin::ELogLevel::Info, "Spotify key saved");
					}
				}
				SecureZeroMemory(strKey.data(), strKey.size());
			}
			Plugin::Api().SameLine();
			if (Plugin::Api().Button("open spotify.kahool.dev"))
				ShellExecuteW(nullptr, L"open", kKeySite, nullptr, nullptr, SW_SHOWNORMAL);
			if (!m_strEntryError.empty())
				Plugin::Api().TextMuted(m_strEntryError.c_str());
			Plugin::Api().TextMuted("the key is encrypted for this Windows user; save your config profile to keep it");
		}

		void DrawCard()
		{
			const stTrack& rTrack = m_stState.stCurrent;
			const std::uint32_t uAccent = m_pArt ? m_uPrimary : Plugin::Api().AccentColor();
			if (rTrack.bValid)
			{
				Plugin::Api().Image(m_pArt, kArtDisplay, kArtDisplay, Plugin::Rgba(255, 255, 255));
				Plugin::Api().SameLine();
				Plugin::Api().BeginGroup();
				Plugin::Api().TextColored(uAccent, rTrack.strTitle.c_str());
				Plugin::Api().Text(rTrack.strArtists.c_str());
				Plugin::Api().TextMuted(rTrack.strAlbum.c_str());
				Plugin::Api().TextMuted(rTrack.bPlaying ? "playing" : "paused");
				Plugin::Api().EndGroup();

				const int iProgress = rTrack.ProgressNow();
				const float flFraction = rTrack.iDurationMs > 0 ? static_cast<float>(iProgress) / static_cast<float>(rTrack.iDurationMs) : 0.f;
				Plugin::Api().ProgressBar(flFraction, std::format("{} / {}", FormatTime(iProgress), FormatTime(rTrack.iDurationMs)).c_str(), uAccent);
			}
			else
				Plugin::Api().TextMuted(m_stState.strStatus.empty() ? "waiting for Spotify" : m_stState.strStatus.c_str());
		}

		void DrawPlayer()
		{
			const stTrack& rTrack = m_stState.stCurrent;
			DrawCard();
			if (Plugin::Api().Button("previous"))
				m_clClient.Command(Spotify::Media::ECommand::Previous);
			Plugin::Api().SameLine();
			if (Plugin::Api().Button(rTrack.bValid && rTrack.bPlaying ? "pause###playpause" : "play###playpause"))
				m_clClient.Command(Spotify::Media::ECommand::PlayPause);
			Plugin::Api().SameLine();
			if (Plugin::Api().Button("next"))
				m_clClient.Command(Spotify::Media::ECommand::Next);
			if (rTrack.bValid && !m_stState.strStatus.empty())
				Plugin::Api().TextMuted(m_stState.strStatus.c_str());

			Plugin::Api().Separator();
			Plugin::Api().SliderFloat("refresh interval", m_pflInterval, 1.f, 10.f, "{:.0f} s");
			if (Plugin::Api().Button("forget key"))
			{
				StoreKey({});
				m_clClient.SetKey({});
				ClearArt();
			}
		}

		clClient m_clClient;
		float* m_pflInterval = nullptr;
		bool* m_pbWindow = nullptr;
		bool* m_pbPinned = nullptr;
		float* m_pflOpacity = nullptr;
		float m_flOverheadX = 0.f;
		float m_flOverheadY = 0.f;
		stSnapshot m_stState;
		std::array<char, 512> m_aKeyInput{};
		std::string m_strStoredKey;
		std::string m_strEntryError;
		bool m_bKeyLoaded = false;
		void* m_pArt = nullptr;
		std::uint32_t m_uPrimary = 0;
	};

	clSpotifyFeature g_clSpotify;

	bool Load()
	{
		return Plugin::RegisterFeature(g_clSpotify);
	}

	void Unload()
	{
	}
}

OMEGAWARE_PLUGIN(Load, Unload)
