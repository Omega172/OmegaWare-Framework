#pragma once

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace Spotify::Library
{
	struct stTrack
	{
		std::string strUri;
		std::string strTitle;
		std::string strArtists;
		int iDurationMs = 0;
	};

	struct stPlaylist
	{
		std::string strId;
		std::string strUri;
		std::string strName;
		int iTracks = 0;
	};

	struct stDevice
	{
		std::string strId;
		std::string strName;
		std::string strType;
		bool bActive = false;
	};

	enum class ESource
	{
		Search,
		Liked,
		Playlist,
	};

	struct stData
	{
		std::vector<stTrack> aSearch;
		std::string strSearchQuery;
		std::vector<stTrack> aLiked;
		int iLikedTotal = 0;
		bool bLikedLoaded = false;
		std::vector<stPlaylist> aPlaylists;
		bool bPlaylistsLoaded = false;
		stPlaylist stOpenPlaylist;
		std::vector<stTrack> aPlaylistTracks;
		int iPlaylistTotal = 0;
		std::string strUser;
		std::unordered_map<std::string, bool> mapLiked;
		std::vector<stDevice> aDevices;
	};

	struct stSnapshot
	{
		std::shared_ptr<const stData> pData;
		std::string strStatus;
		bool bLoggedIn = false;
		bool bLoggingIn = false;
		bool bBusy = false;
		bool bShuffle = false;
		int iRepeat = 0;
		int iVolume = -1;
	};

	class clLibrary
	{
	public:
		clLibrary();
		~clLibrary();

		clLibrary(const clLibrary&) = delete;
		clLibrary& operator=(const clLibrary&) = delete;

		void Start();
		void Stop();

		void SetClientId(std::string strClientId);
		void SetToken(std::string strToken);

		void Login();
		void CancelLogin();
		void Logout();
		void Search(std::string strQuery);
		void LoadLiked(bool bMore);
		void LoadPlaylists();
		void OpenPlaylist(const stPlaylist& rPlaylist);
		void LoadMorePlaylist();
		void Play(ESource eSource, int iIndex);
		void PlayAll(ESource eSource, bool bShuffle);
		void SetShuffle(bool bShuffle);
		void CycleRepeat();
		void RefreshPlayer();
		void ToggleLike(std::string strUri);
		void CheckLiked(std::string strUri);
		void AddToPlaylist(const std::string& strPlaylistId, const std::string& strUri);
		void RemoveFromPlaylist(std::string strUri);
		void Queue(std::string strUri);
		void CreatePlaylist(std::string strName);
		void SetVolume(int iPercent);
		void RefreshDevices();
		void Transfer(std::string strDeviceId);

		[[nodiscard]] stSnapshot Snapshot();
		[[nodiscard]] std::optional<std::string> TakeToken();

		[[nodiscard]] static std::string RedirectUri();

	private:
		struct stImpl;
		std::unique_ptr<stImpl> m_pImpl;
	};
}
