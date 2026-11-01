#include "Library.hpp"

#include "Http.hpp"
#include "Json.hpp"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <Windows.h>
#include <bcrypt.h>
#include <shellapi.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <format>
#include <mutex>
#include <stop_token>
#include <string>
#include <thread>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "bcrypt.lib")
#pragma comment(lib, "shell32.lib")

namespace Spotify::Library
{
	namespace
	{
		using Clock = std::chrono::steady_clock;
		using namespace std::chrono_literals;

		constexpr unsigned short kRedirectPort = 43897;
		constexpr std::string_view kScopes = "user-library-read user-library-modify user-follow-modify playlist-read-private playlist-read-collaborative playlist-modify-public playlist-modify-private user-modify-playback-state user-read-playback-state";
		constexpr wchar_t kTokenUrl[] = L"https://accounts.spotify.com/api/token";
		constexpr std::string_view kApi = "https://api.spotify.com/v1";
		constexpr int kPageSize = 50;
		constexpr int kPlaylistPages = 4;
		constexpr int kQueueBatch = 50;
		constexpr std::chrono::seconds kLoginTimeout{ 180 };

		enum class EJob
		{
			Login,
			Logout,
			Search,
			Liked,
			Playlists,
			OpenPlaylist,
			MorePlaylist,
			Play,
			PlayAll,
			Shuffle,
			Repeat,
			Player,
			ToggleLike,
			CheckLiked,
			AddToPlaylist,
			RemoveFromPlaylist,
			Queue,
			CreatePlaylist,
			Volume,
			Devices,
			Transfer,
		};

		struct stJob
		{
			EJob eKind = EJob::Search;
			std::string strText;
			stPlaylist stList;
			ESource eSource = ESource::Search;
			int iIndex = 0;
			bool bMore = false;
		};

		[[nodiscard]] std::string Base64Url(const unsigned char* pBytes, std::size_t uSize)
		{
			constexpr std::string_view kAlphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
			std::string strOut;
			for (std::size_t uIndex = 0; uIndex < uSize; uIndex += 3)
			{
				const unsigned uChunk = (pBytes[uIndex] << 16) | (uIndex + 1 < uSize ? pBytes[uIndex + 1] << 8 : 0) | (uIndex + 2 < uSize ? pBytes[uIndex + 2] : 0);
				strOut += kAlphabet[(uChunk >> 18) & 63];
				strOut += kAlphabet[(uChunk >> 12) & 63];
				if (uIndex + 1 < uSize)
					strOut += kAlphabet[(uChunk >> 6) & 63];
				if (uIndex + 2 < uSize)
					strOut += kAlphabet[uChunk & 63];
			}
			return strOut;
		}

		[[nodiscard]] std::string RandomText(std::size_t uLength)
		{
			constexpr std::string_view kAlphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-._~";
			std::string strOut(uLength, '\0');
			std::vector<unsigned char> aRandom(uLength);
			BCryptGenRandom(nullptr, aRandom.data(), static_cast<ULONG>(aRandom.size()), BCRYPT_USE_SYSTEM_PREFERRED_RNG);
			for (std::size_t uIndex = 0; uIndex < uLength; ++uIndex)
				strOut[uIndex] = kAlphabet[aRandom[uIndex] % kAlphabet.size()];
			return strOut;
		}

		[[nodiscard]] std::string Challenge(const std::string& strVerifier)
		{
			std::array<unsigned char, 32> aHash{};
			if (BCryptHash(BCRYPT_SHA256_ALG_HANDLE, nullptr, 0, reinterpret_cast<PUCHAR>(const_cast<char*>(strVerifier.data())), static_cast<ULONG>(strVerifier.size()), aHash.data(), static_cast<ULONG>(aHash.size())) != 0)
				return {};
			return Base64Url(aHash.data(), aHash.size());
		}

		[[nodiscard]] std::string UrlDecode(std::string_view strText)
		{
			std::string strOut;
			for (std::size_t uIndex = 0; uIndex < strText.size(); ++uIndex)
			{
				if (strText[uIndex] == '%' && uIndex + 2 < strText.size() + 0 && std::isxdigit(static_cast<unsigned char>(strText[uIndex + 1])) && std::isxdigit(static_cast<unsigned char>(strText[uIndex + 2])))
				{
					strOut += static_cast<char>(std::stoi(std::string(strText.substr(uIndex + 1, 2)), nullptr, 16));
					uIndex += 2;
				}
				else if (strText[uIndex] == '+')
					strOut += ' ';
				else
					strOut += strText[uIndex];
			}
			return strOut;
		}

		[[nodiscard]] std::string QueryValue(std::string_view strQuery, std::string_view strKey)
		{
			std::size_t uStart = 0;
			while (uStart <= strQuery.size())
			{
				std::size_t uEnd = strQuery.find('&', uStart);
				if (uEnd == std::string_view::npos)
					uEnd = strQuery.size();
				const std::string_view strPair = strQuery.substr(uStart, uEnd - uStart);
				const std::size_t uEquals = strPair.find('=');
				if (uEquals != std::string_view::npos && strPair.substr(0, uEquals) == strKey)
					return UrlDecode(strPair.substr(uEquals + 1));
				uStart = uEnd + 1;
			}
			return {};
		}

		[[nodiscard]] std::string JsonEscape(std::string_view strText)
		{
			std::string strOut;
			for (const char chCurrent : strText)
			{
				if (chCurrent == '"' || chCurrent == '\\')
					strOut += '\\';
				if (static_cast<unsigned char>(chCurrent) >= 0x20)
					strOut += chCurrent;
			}
			return strOut;
		}

		[[nodiscard]] std::string ErrorText(const Http::stResponse& rResponse)
		{
			const std::optional<Json::stValue> optRoot = Json::Parse(rResponse.strBody);
			if (optRoot)
			{
				const Json::stValue& rError = (*optRoot)["error"];
				if (!rError["message"].String().empty())
					return std::string(rError["message"].String());
				if (!rError.String().empty())
					return std::string(rError.String());
				if (!(*optRoot)["error_description"].String().empty())
					return std::string((*optRoot)["error_description"].String());
			}
			return {};
		}

		[[nodiscard]] stTrack ParseTrack(const Json::stValue& rTrack)
		{
			stTrack stResult;
			stResult.strUri = std::string(rTrack["uri"].String());
			stResult.strTitle = std::string(rTrack["name"].String());
			for (const Json::stValue& rArtist : rTrack["artists"].aItems)
			{
				if (!stResult.strArtists.empty())
					stResult.strArtists += ", ";
				stResult.strArtists += rArtist["name"].String();
			}
			stResult.iDurationMs = static_cast<int>(rTrack["duration_ms"].Number(0.0));
			return stResult;
		}

		[[nodiscard]] bool IsPlayable(const Json::stValue& rTrack)
		{
			const std::string_view strUri = rTrack["uri"].String();
			return !strUri.empty() && !strUri.starts_with("spotify:local") && rTrack["type"].String() != "episode";
		}
	}

	struct clLibrary::stImpl
	{
		void Start()
		{
			if (thWorker.joinable())
				return;
			pHttp = std::make_unique<Http::clClient>();
			thWorker = std::jthread([this](std::stop_token stToken) { Worker(stToken); });
		}

		void Stop()
		{
			if (!thWorker.joinable())
				return;
			bCancelLogin = true;
			thWorker.request_stop();
			pHttp->Cancel();
			thWorker.join();
			pHttp.reset();
		}

		void Post(stJob stNew)
		{
			{
				std::scoped_lock lckState(mtxState);
				aJobs.push_back(std::move(stNew));
			}
			cvWake.notify_all();
		}

		void SetStatus(std::string strNew)
		{
			std::scoped_lock lckState(mtxState);
			strStatus = std::move(strNew);
		}

		template <typename Fn>
		void Mutate(Fn&& fnChange)
		{
			std::scoped_lock lckState(mtxState);
			auto pNext = std::make_shared<stData>(*pData);
			fnChange(*pNext);
			pData = std::move(pNext);
		}

		[[nodiscard]] std::shared_ptr<const stData> Data()
		{
			std::scoped_lock lckState(mtxState);
			return pData;
		}

		void ForgetToken(const std::string& strMessage)
		{
			{
				std::scoped_lock lckState(mtxState);
				strRefresh.clear();
				strAccess.clear();
				optToken = std::string{};
				pData = std::make_shared<stData>();
				strStatus = strMessage;
			}
		}

		[[nodiscard]] bool StoreTokens(const Json::stValue& rRoot)
		{
			const std::string_view strNewAccess = rRoot["access_token"].String();
			if (strNewAccess.empty())
				return false;
			const double dExpires = std::clamp(rRoot["expires_in"].Number(3600.0), 120.0, 86400.0);
			std::scoped_lock lckState(mtxState);
			strAccess = std::string(strNewAccess);
			tpExpiry = Clock::now() + std::chrono::seconds(static_cast<long long>(dExpires) - 60);
			const std::string_view strRotated = rRoot["refresh_token"].String();
			if (!strRotated.empty() && strRotated != strRefresh)
			{
				strRefresh = std::string(strRotated);
				optToken = strRefresh;
			}
			return true;
		}

		bool EnsureAccess()
		{
			std::string strClient;
			std::string strToken;
			{
				std::scoped_lock lckState(mtxState);
				if (!strAccess.empty() && Clock::now() < tpExpiry)
					return true;
				strClient = strClientId;
				strToken = strRefresh;
			}
			if (strClient.empty() || strToken.empty())
			{
				SetStatus("log in to Spotify first");
				return false;
			}
			const std::string strBody = std::format("grant_type=refresh_token&refresh_token={}&client_id={}", Http::UrlEncode(strToken), Http::UrlEncode(strClient));
			const Http::stResponse stResponse = pHttp->Request(L"POST", kTokenUrl, L"Content-Type: application/x-www-form-urlencoded", strBody, 64 * 1024);
			if (stResponse.uError && !stResponse.uStatus)
			{
				SetStatus("could not reach accounts.spotify.com");
				return false;
			}
			if (stResponse.uStatus == 200)
			{
				const std::optional<Json::stValue> optRoot = Json::Parse(stResponse.strBody);
				if (optRoot && StoreTokens(*optRoot))
					return true;
			}
			if (stResponse.uStatus == 400 || stResponse.uStatus == 401)
			{
				ForgetToken("the library login expired, log in again");
				return false;
			}
			SetStatus(std::format("accounts.spotify.com returned HTTP {}", stResponse.uStatus));
			return false;
		}

		Http::stResponse Call(std::wstring_view strMethod, const std::string& strUrl, std::string_view strBody = {})
		{
			for (int iAttempt = 0; iAttempt < 2; ++iAttempt)
			{
				if (!EnsureAccess())
					return {};
				std::string strBearer;
				{
					std::scoped_lock lckState(mtxState);
					strBearer = strAccess;
				}
				std::string strHeaders = std::format("Authorization: Bearer {}\r\nAccept: application/json", strBearer);
				if (!strBody.empty())
					strHeaders += "\r\nContent-Type: application/json";
				Http::stResponse stResponse = pHttp->Request(strMethod, Http::Widen(strUrl), Http::Widen(strHeaders), strBody, 2 * 1024 * 1024);
				if (stResponse.uStatus == 401 && iAttempt == 0)
				{
					std::scoped_lock lckState(mtxState);
					strAccess.clear();
					continue;
				}
				if (stResponse.uError && !stResponse.uStatus)
					SetStatus("could not reach Spotify");
				else if (stResponse.uStatus == 429)
					SetStatus(std::format("Spotify is rate limiting, try again in {} s", std::clamp(stResponse.iRetryAfter > 0 ? stResponse.iRetryAfter : 5, 1, 600)));
				return stResponse;
			}
			return {};
		}

		[[nodiscard]] std::optional<Json::stValue> Fetch(const std::string& strPath)
		{
			const Http::stResponse stResponse = Call(L"GET", std::string(kApi) + strPath);
			if (stResponse.uStatus == 200)
			{
				std::optional<Json::stValue> optRoot = Json::Parse(stResponse.strBody);
				if (!optRoot)
					SetStatus("Spotify sent a response that could not be read");
				return optRoot;
			}
			if (stResponse.uStatus && stResponse.uStatus != 429)
			{
				const std::string strError = ErrorText(stResponse);
				SetStatus(std::format("Spotify returned HTTP {}{}{}", stResponse.uStatus, strError.empty() ? "" : ": ", strError));
			}
			return std::nullopt;
		}

		void FetchUser()
		{
			if (const std::optional<Json::stValue> optRoot = Fetch("/me"))
			{
				std::string strName = std::string((*optRoot)["display_name"].String());
				if (strName.empty())
					strName = std::string((*optRoot)["id"].String());
				std::string strId = std::string((*optRoot)["id"].String());
				{
					std::scoped_lock lckState(mtxState);
					strUserId = strId;
				}
				Mutate([&](stData& rData) { rData.strUser = strName; });
			}
		}

		void DoSearch(const stJob& rJob)
		{
			if (rJob.strText.empty())
				return;
			SetStatus("searching");
			const std::optional<Json::stValue> optRoot = Fetch(std::format("/search?q={}&type=track&limit=10", Http::UrlEncode(rJob.strText)));
			if (!optRoot)
				return;
			std::vector<stTrack> aFound;
			for (const Json::stValue& rItem : (*optRoot)["tracks"]["items"].aItems)
				if (IsPlayable(rItem))
					aFound.push_back(ParseTrack(rItem));
			const std::size_t uFound = aFound.size();
			std::vector<std::string> aUris;
			for (const stTrack& rTrack : aFound)
				aUris.push_back(rTrack.strUri);
			Mutate([&](stData& rData) {
				rData.aSearch = std::move(aFound);
				rData.strSearchQuery = rJob.strText;
			});
			SetStatus(uFound ? std::string{} : "nothing found");
			FillLiked(aUris);
		}

		void DoLiked(const stJob& rJob)
		{
			const std::shared_ptr<const stData> pCurrent = Data();
			const int iOffset = rJob.bMore ? static_cast<int>(pCurrent->aLiked.size()) : 0;
			SetStatus("loading liked songs");
			const std::optional<Json::stValue> optRoot = Fetch(std::format("/me/tracks?limit={}&offset={}", kPageSize, iOffset));
			if (!optRoot)
				return;
			std::vector<stTrack> aPage;
			for (const Json::stValue& rItem : (*optRoot)["items"].aItems)
				if (IsPlayable(rItem["track"]))
					aPage.push_back(ParseTrack(rItem["track"]));
			const int iTotal = static_cast<int>((*optRoot)["total"].Number(0.0));
			Mutate([&](stData& rData) {
				if (!rJob.bMore)
					rData.aLiked.clear();
				for (const stTrack& rTrack : aPage)
					rData.mapLiked[rTrack.strUri] = true;
				rData.aLiked.insert(rData.aLiked.end(), aPage.begin(), aPage.end());
				rData.iLikedTotal = iTotal;
				rData.bLikedLoaded = true;
			});
			SetStatus({});
		}

		void DoPlaylists()
		{
			SetStatus("loading playlists");
			std::string strUser;
			{
				std::scoped_lock lckState(mtxState);
				strUser = strUserId;
			}
			if (strUser.empty())
			{
				FetchUser();
				std::scoped_lock lckState(mtxState);
				strUser = strUserId;
			}
			std::vector<stPlaylist> aLists;
			for (int iPage = 0; iPage < kPlaylistPages; ++iPage)
			{
				const std::optional<Json::stValue> optRoot = Fetch(std::format("/me/playlists?limit={}&offset={}", kPageSize, iPage * kPageSize));
				if (!optRoot)
					return;
				const auto& rItems = (*optRoot)["items"].aItems;
				for (const Json::stValue& rItem : rItems)
				{
					if (rItem["owner"]["id"].String() != strUser)
						continue;
					stPlaylist stNew;
					stNew.strId = std::string(rItem["id"].String());
					stNew.strUri = std::string(rItem["uri"].String());
					stNew.strName = std::string(rItem["name"].String());
					const Json::stValue& rCount = rItem["items"].IsNull() ? rItem["tracks"] : rItem["items"];
					stNew.iTracks = static_cast<int>(rCount["total"].Number(0.0));
					if (!stNew.strId.empty())
						aLists.push_back(std::move(stNew));
				}
				if (static_cast<int>(rItems.size()) < kPageSize)
					break;
			}
			Mutate([&](stData& rData) {
				rData.aPlaylists = std::move(aLists);
				rData.bPlaylistsLoaded = true;
			});
			SetStatus({});
		}

		void DoPlaylistPage(const stJob& rJob)
		{
			const std::shared_ptr<const stData> pCurrent = Data();
			const stPlaylist stList = rJob.eKind == EJob::OpenPlaylist ? rJob.stList : pCurrent->stOpenPlaylist;
			if (stList.strId.empty())
				return;
			const bool bFirst = rJob.eKind == EJob::OpenPlaylist;
			if (bFirst)
				Mutate([&](stData& rData) {
					rData.stOpenPlaylist = stList;
					rData.aPlaylistTracks.clear();
					rData.iPlaylistTotal = stList.iTracks;
				});
			const int iOffset = bFirst ? 0 : static_cast<int>(pCurrent->aPlaylistTracks.size());
			SetStatus("loading playlist");
			const std::optional<Json::stValue> optRoot = Fetch(std::format("/playlists/{}/items?limit={}&offset={}", stList.strId, kPageSize, iOffset));
			if (!optRoot)
				return;
			std::vector<stTrack> aPage;
			for (const Json::stValue& rItem : (*optRoot)["items"].aItems)
			{
				const Json::stValue& rTrack = rItem["item"].IsNull() ? rItem["track"] : rItem["item"];
				if (IsPlayable(rTrack))
					aPage.push_back(ParseTrack(rTrack));
			}
			const int iTotal = static_cast<int>((*optRoot)["total"].Number(0.0));
			std::vector<std::string> aUris;
			for (const stTrack& rTrack : aPage)
				aUris.push_back(rTrack.strUri);
			Mutate([&](stData& rData) {
				rData.aPlaylistTracks.insert(rData.aPlaylistTracks.end(), aPage.begin(), aPage.end());
				rData.iPlaylistTotal = iTotal;
			});
			SetStatus({});
			FillLiked(aUris);
		}

		void DoPlay(const stJob& rJob)
		{
			const std::shared_ptr<const stData> pCurrent = Data();
			const std::vector<stTrack>& rList = rJob.eSource == ESource::Search ? pCurrent->aSearch : (rJob.eSource == ESource::Liked ? pCurrent->aLiked : pCurrent->aPlaylistTracks);
			if (rJob.iIndex < 0 || rJob.iIndex >= static_cast<int>(rList.size()))
				return;
			const stTrack& rTrack = rList[static_cast<std::size_t>(rJob.iIndex)];
			std::string strBody;
			if (rJob.eSource == ESource::Playlist)
				strBody = std::format("{{\"context_uri\":\"{}\",\"offset\":{{\"uri\":\"{}\"}}}}", JsonEscape(pCurrent->stOpenPlaylist.strUri), JsonEscape(rTrack.strUri));
			else
			{
				strBody = "{\"uris\":[";
				const std::size_t uEnd = rJob.eSource == ESource::Liked ? std::min(rList.size(), static_cast<std::size_t>(rJob.iIndex + kQueueBatch)) : static_cast<std::size_t>(rJob.iIndex + 1);
				for (std::size_t uIndex = static_cast<std::size_t>(rJob.iIndex); uIndex < uEnd; ++uIndex)
				{
					if (uIndex != static_cast<std::size_t>(rJob.iIndex))
						strBody += ',';
					strBody += std::format("\"{}\"", JsonEscape(rList[uIndex].strUri));
				}
				strBody += "]}";
			}

			if (PlayerPut("/me/player/play", strBody))
				SetStatus(std::format("playing {}", rTrack.strTitle));
			ReadPlayer();
		}

		void ReportError(const Http::stResponse& rResponse)
		{
			const std::string strError = ErrorText(rResponse);
			std::string strLower = strError;
			std::ranges::transform(strLower, strLower.begin(), [](unsigned char chLetter) { return static_cast<char>(std::tolower(chLetter)); });
			if (rResponse.uStatus == 403 && strLower.find("scope") != std::string::npos)
				SetStatus("log out of the library and log in again to grant the new permissions");
			else if (rResponse.uStatus == 403 && strLower.find("premium") != std::string::npos)
				SetStatus("that needs Spotify Premium");
			else if (rResponse.uStatus != 429)
				SetStatus(std::format("Spotify returned HTTP {}{}{}", rResponse.uStatus, strError.empty() ? "" : ": ", strError));
		}

		bool Send(std::wstring_view strMethod, const std::string& strPath, std::string_view strBody = {})
		{
			const Http::stResponse stResponse = Call(strMethod, std::string(kApi) + strPath, strBody);
			if (stResponse.uStatus >= 200 && stResponse.uStatus < 300)
				return true;
			if (stResponse.uStatus)
				ReportError(stResponse);
			return false;
		}

		bool PlayerPut(const std::string& strPath, const std::string& strBody, std::wstring_view strMethod = L"PUT")
		{
			const std::string strUrl = std::string(kApi) + strPath;
			const char chJoin = strPath.find('?') == std::string::npos ? '?' : '&';
			Http::stResponse stResponse = Call(strMethod, strUrl, strBody);
			if (stResponse.uStatus == 404)
			{
				const std::string strDevice = PickDevice();
				if (strDevice.empty())
				{
					SetStatus("no Spotify device found, open the Spotify app on this PC");
					return false;
				}
				stResponse = Call(strMethod, std::format("{}{}device_id={}", strUrl, chJoin, Http::UrlEncode(strDevice)), strBody);
			}
			if (stResponse.uStatus == 200 || stResponse.uStatus == 202 || stResponse.uStatus == 204)
				return true;
			if (!stResponse.uStatus)
				return false;
			ReportError(stResponse);
			return false;
		}

		void ReadPlayer()
		{
			const Http::stResponse stResponse = Call(L"GET", std::string(kApi) + "/me/player");
			if (stResponse.uStatus != 200)
				return;
			const std::optional<Json::stValue> optRoot = Json::Parse(stResponse.strBody);
			if (!optRoot)
				return;
			const std::string_view strRepeat = (*optRoot)["repeat_state"].String();
			bShuffle = (*optRoot)["shuffle_state"].Bool();
			iRepeat = strRepeat == "context" ? 1 : (strRepeat == "track" ? 2 : 0);
			iVolume = static_cast<int>((*optRoot)["device"]["volume_percent"].Number(-1.0));
		}

		void DoPlayAll(const stJob& rJob)
		{
			std::string strContext;
			std::string strName;
			if (rJob.eSource == ESource::Playlist)
			{
				const std::shared_ptr<const stData> pCurrent = Data();
				strContext = pCurrent->stOpenPlaylist.strUri;
				strName = pCurrent->stOpenPlaylist.strName;
			}
			else
			{
				std::string strUser;
				{
					std::scoped_lock lckState(mtxState);
					strUser = strUserId;
				}
				if (strUser.empty())
				{
					FetchUser();
					std::scoped_lock lckState(mtxState);
					strUser = strUserId;
				}
				strContext = std::format("spotify:user:{}:collection", strUser);
				strName = "liked songs";
			}
			if (strContext.empty())
				return;
			if (rJob.bMore != bShuffle)
			{
				if (!PlayerPut(std::format("/me/player/shuffle?state={}", rJob.bMore ? "true" : "false"), {}))
					return;
				std::this_thread::sleep_for(350ms);
			}
			if (PlayerPut("/me/player/play", std::format("{{\"context_uri\":\"{}\"}}", JsonEscape(strContext))))
				SetStatus(std::format("playing {}{}", strName, rJob.bMore ? " (shuffled)" : ""));
			ReadPlayer();
		}

		void DoShuffle(const stJob& rJob)
		{
			if (PlayerPut(std::format("/me/player/shuffle?state={}", rJob.bMore ? "true" : "false"), {}))
				SetStatus({});
			ReadPlayer();
		}

		void DoRepeat()
		{
			ReadPlayer();
			constexpr std::array<std::string_view, 3> aStates = { "off", "context", "track" };
			if (PlayerPut(std::format("/me/player/repeat?state={}", aStates[static_cast<std::size_t>((iRepeat + 1) % 3)]), {}))
				SetStatus({});
			ReadPlayer();
		}

		[[nodiscard]] std::string PickDevice()
		{
			const std::optional<Json::stValue> optRoot = Fetch("/me/player/devices");
			if (!optRoot)
				return {};
			std::string strActive;
			std::string strComputer;
			std::string strAny;
			for (const Json::stValue& rDevice : (*optRoot)["devices"].aItems)
			{
				const std::string strId = std::string(rDevice["id"].String());
				if (strId.empty())
					continue;
				if (strAny.empty())
					strAny = strId;
				if (rDevice["is_active"].Bool() && strActive.empty())
					strActive = strId;
				if (rDevice["type"].String() == "Computer" && strComputer.empty())
					strComputer = strId;
			}
			if (!strActive.empty())
				return strActive;
			return !strComputer.empty() ? strComputer : strAny;
		}

		void DoLogin(std::stop_token stToken)
		{
			std::string strClient;
			{
				std::scoped_lock lckState(mtxState);
				strClient = strClientId;
			}
			if (strClient.empty())
			{
				SetStatus("enter your Spotify client ID first");
				return;
			}

			WSADATA stWsa{};
			if (WSAStartup(MAKEWORD(2, 2), &stWsa) != 0)
			{
				SetStatus("could not start the local login listener");
				return;
			}
			SOCKET sListen = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
			sockaddr_in stAddress{};
			stAddress.sin_family = AF_INET;
			stAddress.sin_port = htons(kRedirectPort);
			inet_pton(AF_INET, "127.0.0.1", &stAddress.sin_addr);
			if (sListen == INVALID_SOCKET || bind(sListen, reinterpret_cast<sockaddr*>(&stAddress), sizeof(stAddress)) != 0 || listen(sListen, 4) != 0)
			{
				SetStatus(std::format("port {} is in use, close whatever is using it and try again", kRedirectPort));
				if (sListen != INVALID_SOCKET)
					closesocket(sListen);
				WSACleanup();
				return;
			}

			const std::string strVerifier = RandomText(96);
			const std::string strState = RandomText(24);
			const std::string strRedirect = RedirectUri();
			const std::string strUrl = std::format("https://accounts.spotify.com/authorize?response_type=code&client_id={}&scope={}&redirect_uri={}&state={}&code_challenge_method=S256&code_challenge={}",
				Http::UrlEncode(strClient), Http::UrlEncode(kScopes), Http::UrlEncode(strRedirect), Http::UrlEncode(strState), Challenge(strVerifier));
			bCancelLogin = false;
			bLoggingIn = true;
			SetStatus("waiting for the browser login");
			ShellExecuteW(nullptr, L"open", Http::Widen(strUrl).c_str(), nullptr, nullptr, SW_SHOWNORMAL);

			std::string strCode;
			std::string strFailure;
			const Clock::time_point tpDeadline = Clock::now() + kLoginTimeout;
			while (strCode.empty() && strFailure.empty())
			{
				if (stToken.stop_requested() || bCancelLogin)
				{
					strFailure = "login cancelled";
					break;
				}
				if (Clock::now() > tpDeadline)
				{
					strFailure = "login timed out";
					break;
				}
				fd_set stSet;
				FD_ZERO(&stSet);
				FD_SET(sListen, &stSet);
				timeval stWait{ 0, 250000 };
				if (select(0, &stSet, nullptr, nullptr, &stWait) <= 0)
					continue;
				SOCKET sClient = accept(sListen, nullptr, nullptr);
				if (sClient == INVALID_SOCKET)
					continue;
				DWORD uTimeout = 2000;
				setsockopt(sClient, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&uTimeout), sizeof(uTimeout));
				std::array<char, 4096> aRequest{};
				const int iRead = recv(sClient, aRequest.data(), static_cast<int>(aRequest.size() - 1), 0);
				bool bCallback = false;
				if (iRead > 0)
				{
					const std::string_view strRequest(aRequest.data(), static_cast<std::size_t>(iRead));
					const std::size_t uLineEnd = strRequest.find("\r\n");
					const std::string_view strLine = strRequest.substr(0, uLineEnd);
					if (strLine.starts_with("GET /callback"))
					{
						bCallback = true;
						const std::size_t uQuery = strLine.find('?');
						const std::size_t uSpace = strLine.rfind(' ');
						const std::string_view strQuery = uQuery == std::string_view::npos ? std::string_view{} : strLine.substr(uQuery + 1, uSpace > uQuery ? uSpace - uQuery - 1 : 0);
						if (QueryValue(strQuery, "state") != strState)
							strFailure = "login rejected: the state did not match";
						else if (!QueryValue(strQuery, "error").empty())
							strFailure = std::format("login refused: {}", QueryValue(strQuery, "error"));
						else
							strCode = QueryValue(strQuery, "code");
						if (strCode.empty() && strFailure.empty())
							strFailure = "login returned no code";
					}
				}
				const std::string strPage = bCallback ? (strFailure.empty() ? "<html><body style=\"font-family:sans-serif\"><h3>Logged in</h3>You can close this tab and go back to the game.</body></html>" : "<html><body style=\"font-family:sans-serif\"><h3>Login failed</h3>You can close this tab.</body></html>") : std::string{};
				const std::string strReply = std::format("HTTP/1.1 {} X\r\nContent-Type: text/html; charset=utf-8\r\nContent-Length: {}\r\nConnection: close\r\n\r\n{}", bCallback ? 200 : 404, strPage.size(), strPage);
				send(sClient, strReply.data(), static_cast<int>(strReply.size()), 0);
				closesocket(sClient);
			}
			closesocket(sListen);
			WSACleanup();
			bLoggingIn = false;
			if (!strFailure.empty())
			{
				SetStatus(strFailure);
				return;
			}

			const std::string strBody = std::format("grant_type=authorization_code&code={}&redirect_uri={}&client_id={}&code_verifier={}", Http::UrlEncode(strCode), Http::UrlEncode(strRedirect), Http::UrlEncode(strClient), Http::UrlEncode(strVerifier));
			const Http::stResponse stResponse = pHttp->Request(L"POST", kTokenUrl, L"Content-Type: application/x-www-form-urlencoded", strBody, 64 * 1024);
			if (stResponse.uStatus != 200)
			{
				const std::string strError = ErrorText(stResponse);
				SetStatus(std::format("login failed (HTTP {}){}{}", stResponse.uStatus, strError.empty() ? "" : ": ", strError));
				return;
			}
			const std::optional<Json::stValue> optRoot = Json::Parse(stResponse.strBody);
			if (!optRoot || !StoreTokens(*optRoot))
			{
				SetStatus("login failed: Spotify sent no token");
				return;
			}
			{
				std::scoped_lock lckState(mtxState);
				if (strRefresh.empty())
				{
					strRefresh = std::string((*optRoot)["refresh_token"].String());
					optToken = strRefresh;
				}
			}
			SetStatus({});
			FetchUser();
		}

		static std::string JoinUris(const std::vector<std::string>& aUris, std::size_t uBegin, std::size_t uEnd)
		{
			std::string strJoined;
			for (std::size_t uIndex = uBegin; uIndex < uEnd; ++uIndex)
			{
				if (!strJoined.empty())
					strJoined += ',';
				strJoined += aUris[uIndex];
			}
			return strJoined;
		}

		void FillLiked(const std::vector<std::string>& aUris)
		{
			const std::shared_ptr<const stData> pCurrent = Data();
			std::vector<std::string> aMissing;
			for (const std::string& rUri : aUris)
				if (!rUri.empty() && !pCurrent->mapLiked.contains(rUri) && std::ranges::find(aMissing, rUri) == aMissing.end())
					aMissing.push_back(rUri);
			for (std::size_t uBegin = 0; uBegin < aMissing.size(); uBegin += 40)
			{
				const std::size_t uEnd = std::min(aMissing.size(), uBegin + 40);
				const std::optional<Json::stValue> optRoot = Fetch("/me/library/contains?uris=" + Http::UrlEncode(JoinUris(aMissing, uBegin, uEnd)));
				if (!optRoot || optRoot->aItems.size() != uEnd - uBegin)
					return;
				Mutate([&](stData& rData) {
					for (std::size_t uIndex = uBegin; uIndex < uEnd; ++uIndex)
						rData.mapLiked[aMissing[uIndex]] = optRoot->aItems[uIndex - uBegin].Bool();
				});
			}
		}

		void DoToggleLike(const stJob& rJob)
		{
			if (rJob.strText.empty())
				return;
			FillLiked({ rJob.strText });
			const std::shared_ptr<const stData> pCurrent = Data();
			const auto itLiked = pCurrent->mapLiked.find(rJob.strText);
			if (itLiked == pCurrent->mapLiked.end())
				return;
			const bool bNow = !itLiked->second;
			if (!Send(bNow ? L"PUT" : L"DELETE", "/me/library?uris=" + Http::UrlEncode(rJob.strText)))
				return;
			Mutate([&](stData& rData) {
				rData.mapLiked[rJob.strText] = bNow;
				if (!bNow)
				{
					std::erase_if(rData.aLiked, [&](const stTrack& rTrack) { return rTrack.strUri == rJob.strText; });
					rData.iLikedTotal = std::max(0, rData.iLikedTotal - 1);
				}
			});
			SetStatus(bNow ? "added to your liked songs" : "removed from your liked songs");
		}

		void DoAddToPlaylist(const stJob& rJob)
		{
			if (rJob.stList.strId.empty() || rJob.strText.empty())
				return;
			if (!Send(L"POST", std::format("/playlists/{}/items", rJob.stList.strId), std::format("{{\"uris\":[\"{}\"]}}", JsonEscape(rJob.strText))))
				return;
			const std::shared_ptr<const stData> pCurrent = Data();
			std::string strName = rJob.stList.strId;
			for (const stPlaylist& rList : pCurrent->aPlaylists)
				if (rList.strId == rJob.stList.strId)
					strName = rList.strName;
			Mutate([&](stData& rData) {
				for (stPlaylist& rList : rData.aPlaylists)
					if (rList.strId == rJob.stList.strId)
						++rList.iTracks;
			});
			SetStatus(std::format("added to {}", strName));
			if (pCurrent->stOpenPlaylist.strId == rJob.stList.strId)
			{
				stJob stReload;
				stReload.eKind = EJob::OpenPlaylist;
				stReload.stList = pCurrent->stOpenPlaylist;
				DoPlaylistPage(stReload);
				SetStatus(std::format("added to {}", strName));
			}
		}

		void DoRemoveFromPlaylist(const stJob& rJob)
		{
			const std::shared_ptr<const stData> pCurrent = Data();
			const stPlaylist stList = pCurrent->stOpenPlaylist;
			if (stList.strId.empty() || rJob.strText.empty())
				return;
			if (!Send(L"DELETE", std::format("/playlists/{}/items", stList.strId), std::format("{{\"items\":[{{\"uri\":\"{}\"}}]}}", JsonEscape(rJob.strText))))
				return;
			Mutate([&](stData& rData) {
				const std::size_t uBefore = rData.aPlaylistTracks.size();
				std::erase_if(rData.aPlaylistTracks, [&](const stTrack& rTrack) { return rTrack.strUri == rJob.strText; });
				const int iRemoved = static_cast<int>(uBefore - rData.aPlaylistTracks.size());
				rData.iPlaylistTotal = std::max(0, rData.iPlaylistTotal - iRemoved);
				for (stPlaylist& rList : rData.aPlaylists)
					if (rList.strId == stList.strId)
						rList.iTracks = std::max(0, rList.iTracks - iRemoved);
			});
			SetStatus(std::format("removed from {}", stList.strName));
		}

		void DoCreatePlaylist(const stJob& rJob)
		{
			if (rJob.strText.empty())
				return;
			if (!Send(L"POST", "/me/playlists", std::format("{{\"name\":\"{}\",\"public\":false}}", JsonEscape(rJob.strText))))
				return;
			DoPlaylists();
			SetStatus(std::format("created {}", rJob.strText));
		}

		void DoDevices()
		{
			const std::optional<Json::stValue> optRoot = Fetch("/me/player/devices");
			if (!optRoot)
				return;
			std::vector<stDevice> aDevices;
			for (const Json::stValue& rDevice : (*optRoot)["devices"].aItems)
			{
				if (rDevice["id"].String().empty())
					continue;
				aDevices.push_back({ std::string(rDevice["id"].String()), std::string(rDevice["name"].String()), std::string(rDevice["type"].String()), rDevice["is_active"].Bool() });
			}
			Mutate([&](stData& rData) { rData.aDevices = std::move(aDevices); });
		}

		void Run(const stJob& rJob, std::stop_token stToken)
		{
			switch (rJob.eKind)
			{
			case EJob::Login: DoLogin(stToken); break;
			case EJob::Logout: ForgetToken({}); break;
			case EJob::Search: DoSearch(rJob); break;
			case EJob::Liked: DoLiked(rJob); break;
			case EJob::Playlists: DoPlaylists(); break;
			case EJob::OpenPlaylist:
			case EJob::MorePlaylist: DoPlaylistPage(rJob); break;
			case EJob::Play: DoPlay(rJob); break;
			case EJob::PlayAll: DoPlayAll(rJob); break;
			case EJob::Shuffle: DoShuffle(rJob); break;
			case EJob::Repeat: DoRepeat(); break;
			case EJob::Player: ReadPlayer(); DoDevices(); break;
			case EJob::ToggleLike: DoToggleLike(rJob); break;
			case EJob::CheckLiked: FillLiked({ rJob.strText }); break;
			case EJob::AddToPlaylist: DoAddToPlaylist(rJob); break;
			case EJob::RemoveFromPlaylist: DoRemoveFromPlaylist(rJob); break;
			case EJob::Queue:
				if (PlayerPut("/me/player/queue?uri=" + Http::UrlEncode(rJob.strText), {}, L"POST"))
					SetStatus("added to the queue");
				break;
			case EJob::CreatePlaylist: DoCreatePlaylist(rJob); break;
			case EJob::Volume:
				if (PlayerPut(std::format("/me/player/volume?volume_percent={}", std::clamp(rJob.iIndex, 0, 100)), {}))
					iVolume = std::clamp(rJob.iIndex, 0, 100);
				break;
			case EJob::Devices: DoDevices(); break;
			case EJob::Transfer:
				if (Send(L"PUT", "/me/player", std::format("{{\"device_ids\":[\"{}\"],\"play\":true}}", JsonEscape(rJob.strText))))
					SetStatus("moved playback");
				DoDevices();
				ReadPlayer();
				break;
			}
		}

		void Worker(std::stop_token stToken)
		{
			while (!stToken.stop_requested())
			{
				stJob stNext;
				{
					std::unique_lock lckState(mtxState);
					cvWake.wait(lckState, stToken, [this] { return !aJobs.empty(); });
					if (stToken.stop_requested())
						break;
					stNext = std::move(aJobs.front());
					aJobs.pop_front();
					bWorking = true;
				}
				Run(stNext, stToken);
				std::scoped_lock lckState(mtxState);
				bWorking = false;
			}
		}

		std::unique_ptr<Http::clClient> pHttp;
		std::jthread thWorker;
		std::mutex mtxState;
		std::condition_variable_any cvWake;
		std::deque<stJob> aJobs;
		std::shared_ptr<const stData> pData = std::make_shared<stData>();
		std::string strStatus;
		std::string strClientId;
		std::string strRefresh;
		std::string strAccess;
		std::string strUserId;
		Clock::time_point tpExpiry{};
		std::optional<std::string> optToken;
		bool bWorking = false;
		std::atomic<bool> bShuffle = false;
		std::atomic<int> iRepeat = 0;
		std::atomic<int> iVolume = -1;
		std::atomic<bool> bCancelLogin = false;
		std::atomic<bool> bLoggingIn = false;
	};

	clLibrary::clLibrary() : m_pImpl(std::make_unique<stImpl>()) {}

	clLibrary::~clLibrary()
	{
		Stop();
	}

	void clLibrary::Start()
	{
		m_pImpl->Start();
	}

	void clLibrary::Stop()
	{
		m_pImpl->Stop();
	}

	void clLibrary::SetClientId(std::string strClientId)
	{
		std::scoped_lock lckState(m_pImpl->mtxState);
		m_pImpl->strClientId = std::move(strClientId);
	}

	void clLibrary::SetToken(std::string strToken)
	{
		std::scoped_lock lckState(m_pImpl->mtxState);
		if (m_pImpl->strRefresh == strToken)
			return;
		m_pImpl->strRefresh = std::move(strToken);
		m_pImpl->strAccess.clear();
	}

	void clLibrary::Login()
	{
		m_pImpl->Post({ .eKind = EJob::Login });
	}

	void clLibrary::CancelLogin()
	{
		m_pImpl->bCancelLogin = true;
	}

	void clLibrary::Logout()
	{
		m_pImpl->Post({ .eKind = EJob::Logout });
	}

	void clLibrary::Search(std::string strQuery)
	{
		m_pImpl->Post({ .eKind = EJob::Search, .strText = std::move(strQuery) });
	}

	void clLibrary::LoadLiked(bool bMore)
	{
		m_pImpl->Post({ .eKind = EJob::Liked, .bMore = bMore });
	}

	void clLibrary::LoadPlaylists()
	{
		m_pImpl->Post({ .eKind = EJob::Playlists });
	}

	void clLibrary::OpenPlaylist(const stPlaylist& rPlaylist)
	{
		m_pImpl->Post({ .eKind = EJob::OpenPlaylist, .stList = rPlaylist });
	}

	void clLibrary::LoadMorePlaylist()
	{
		m_pImpl->Post({ .eKind = EJob::MorePlaylist });
	}

	void clLibrary::Play(ESource eSource, int iIndex)
	{
		m_pImpl->Post({ .eKind = EJob::Play, .eSource = eSource, .iIndex = iIndex });
	}

	void clLibrary::PlayAll(ESource eSource, bool bShuffle)
	{
		m_pImpl->Post({ .eKind = EJob::PlayAll, .eSource = eSource, .bMore = bShuffle });
	}

	void clLibrary::SetShuffle(bool bShuffle)
	{
		m_pImpl->Post({ .eKind = EJob::Shuffle, .bMore = bShuffle });
	}

	void clLibrary::CycleRepeat()
	{
		m_pImpl->Post({ .eKind = EJob::Repeat });
	}

	void clLibrary::RefreshPlayer()
	{
		m_pImpl->Post({ .eKind = EJob::Player });
	}

	void clLibrary::ToggleLike(std::string strUri)
	{
		m_pImpl->Post({ .eKind = EJob::ToggleLike, .strText = std::move(strUri) });
	}

	void clLibrary::CheckLiked(std::string strUri)
	{
		m_pImpl->Post({ .eKind = EJob::CheckLiked, .strText = std::move(strUri) });
	}

	void clLibrary::AddToPlaylist(const std::string& strPlaylistId, const std::string& strUri)
	{
		stPlaylist stList;
		stList.strId = strPlaylistId;
		m_pImpl->Post({ .eKind = EJob::AddToPlaylist, .strText = strUri, .stList = std::move(stList) });
	}

	void clLibrary::RemoveFromPlaylist(std::string strUri)
	{
		m_pImpl->Post({ .eKind = EJob::RemoveFromPlaylist, .strText = std::move(strUri) });
	}

	void clLibrary::Queue(std::string strUri)
	{
		m_pImpl->Post({ .eKind = EJob::Queue, .strText = std::move(strUri) });
	}

	void clLibrary::CreatePlaylist(std::string strName)
	{
		m_pImpl->Post({ .eKind = EJob::CreatePlaylist, .strText = std::move(strName) });
	}

	void clLibrary::SetVolume(int iPercent)
	{
		m_pImpl->Post({ .eKind = EJob::Volume, .iIndex = iPercent });
	}

	void clLibrary::RefreshDevices()
	{
		m_pImpl->Post({ .eKind = EJob::Devices });
	}

	void clLibrary::Transfer(std::string strDeviceId)
	{
		m_pImpl->Post({ .eKind = EJob::Transfer, .strText = std::move(strDeviceId) });
	}

	stSnapshot clLibrary::Snapshot()
	{
		std::scoped_lock lckState(m_pImpl->mtxState);
		return { m_pImpl->pData, m_pImpl->strStatus, !m_pImpl->strRefresh.empty(), m_pImpl->bLoggingIn.load(), m_pImpl->bWorking || !m_pImpl->aJobs.empty(), m_pImpl->bShuffle.load(), m_pImpl->iRepeat.load(), m_pImpl->iVolume.load() };
	}

	std::optional<std::string> clLibrary::TakeToken()
	{
		std::scoped_lock lckState(m_pImpl->mtxState);
		return std::exchange(m_pImpl->optToken, std::nullopt);
	}

	std::string clLibrary::RedirectUri()
	{
		return std::format("http://127.0.0.1:{}/callback", kRedirectPort);
	}
}
