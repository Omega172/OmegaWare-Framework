#include "Media.hpp"

#include <Windows.h>

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Media.Control.h>

#include <cwctype>
#include <string>

#pragma comment(lib, "windowsapp.lib")

namespace Spotify::Media
{
	namespace
	{
		using namespace winrt::Windows::Media::Control;

		[[nodiscard]] bool IsSpotify(const GlobalSystemMediaTransportControlsSession& rSession)
		{
			std::wstring strId(rSession.SourceAppUserModelId());
			for (wchar_t& rChar : strId)
				rChar = static_cast<wchar_t>(std::towlower(rChar));
			return strId.find(L"spotify") != std::wstring::npos;
		}
	}

	struct clController::stImpl
	{
		GlobalSystemMediaTransportControlsSessionManager clManager{ nullptr };
	};

	clController::clController() : m_pImpl(std::make_unique<stImpl>()) {}

	clController::~clController() = default;

	EResult clController::Send(ECommand eCommand)
	{
		try
		{
			if (!m_pImpl->clManager)
				m_pImpl->clManager = GlobalSystemMediaTransportControlsSessionManager::RequestAsync().get();
			GlobalSystemMediaTransportControlsSession clSession{ nullptr };
			for (const GlobalSystemMediaTransportControlsSession& rSession : m_pImpl->clManager.GetSessions())
			{
				if (IsSpotify(rSession))
				{
					clSession = rSession;
					break;
				}
			}
			if (!clSession)
				return EResult::NoSession;
			bool bSent = false;
			switch (eCommand)
			{
			case ECommand::PlayPause: bSent = clSession.TryTogglePlayPauseAsync().get(); break;
			case ECommand::Next: bSent = clSession.TrySkipNextAsync().get(); break;
			case ECommand::Previous: bSent = clSession.TrySkipPreviousAsync().get(); break;
			}
			return bSent ? EResult::Sent : EResult::Failed;
		}
		catch (const winrt::hresult_error&)
		{
			m_pImpl->clManager = nullptr;
			return EResult::Failed;
		}
	}
}
