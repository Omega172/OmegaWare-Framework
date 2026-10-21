#include "Core/Console.hpp"
#include "Core/Framework.hpp"

#include <Windows.h>

#include <deque>
#include <mutex>
#include <string>
#include <utility>

namespace OmegaWare::Console
{
	namespace
	{
		constexpr std::size_t kBacklogLines = 1000;

		std::recursive_mutex g_mtxConsole;
		std::deque<std::pair<OmegaRenderHook::ELogLevel, std::string>> g_aBacklog;
		std::wstring g_strTitle = L"" OMEGAWARE_TITLE;
		HANDLE g_hOutput = INVALID_HANDLE_VALUE;
		HWND g_hWindow = nullptr;
		bool g_bOwned = false;
		bool g_bVisible = false;

		[[nodiscard]] std::wstring Widen(std::string_view strText)
		{
			if (strText.empty())
				return {};
			const int iLength = MultiByteToWideChar(CP_UTF8, 0, strText.data(), static_cast<int>(strText.size()), nullptr, 0);
			std::wstring strWide(static_cast<std::size_t>(iLength), L'\0');
			MultiByteToWideChar(CP_UTF8, 0, strText.data(), static_cast<int>(strText.size()), strWide.data(), iLength);
			return strWide;
		}

		[[nodiscard]] std::wstring_view ColorCode(OmegaRenderHook::ELogLevel eLevel)
		{
			switch (eLevel)
			{
			case OmegaRenderHook::ELogLevel::Debug: return L"\x1b[90m";
			case OmegaRenderHook::ELogLevel::Warning: return L"\x1b[33m";
			case OmegaRenderHook::ELogLevel::Error: return L"\x1b[31m";
			default: return L"\x1b[0m";
			}
		}

		void WriteLine(OmegaRenderHook::ELogLevel eLevel, std::string_view strLine)
		{
			if (g_hOutput == INVALID_HANDLE_VALUE)
				return;
			const std::wstring strText = std::wstring(ColorCode(eLevel)) + Widen(strLine) + L"\x1b[0m\n";
			DWORD uWritten = 0;
			WriteConsoleW(g_hOutput, strText.data(), static_cast<DWORD>(strText.size()), &uWritten, nullptr);
		}

		BOOL WINAPI CtrlHandler(DWORD uEvent)
		{
			return uEvent == CTRL_C_EVENT || uEvent == CTRL_BREAK_EVENT;
		}

		bool Create()
		{
			if (g_hWindow)
				return true;

			g_bOwned = AllocConsole() != FALSE;
			g_hWindow = GetConsoleWindow();
			if (!g_hWindow)
				return false;

			g_hOutput = CreateFileW(L"CONOUT$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
			DWORD uMode = 0;
			if (g_hOutput != INVALID_HANDLE_VALUE && GetConsoleMode(g_hOutput, &uMode))
				SetConsoleMode(g_hOutput, uMode | ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING);

			if (g_bOwned)
			{
				const HANDLE hInput = CreateFileW(L"CONIN$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
				if (hInput != INVALID_HANDLE_VALUE)
				{
					if (GetConsoleMode(hInput, &uMode))
						SetConsoleMode(hInput, (uMode | ENABLE_EXTENDED_FLAGS) & ~ENABLE_QUICK_EDIT_MODE);
					CloseHandle(hInput);
				}
				if (HMENU hMenu = GetSystemMenu(g_hWindow, FALSE))
					DeleteMenu(hMenu, SC_CLOSE, MF_BYCOMMAND);
				SetConsoleCtrlHandler(&CtrlHandler, TRUE);
				SetConsoleTitleW(g_strTitle.c_str());
			}

			for (const auto& [eLevel, strLine] : g_aBacklog)
				WriteLine(eLevel, strLine);
			return true;
		}
	}

	void SetTitle(std::string_view strTitle)
	{
		std::scoped_lock lckConsole(g_mtxConsole);
		g_strTitle = Widen(strTitle);
		if (g_hWindow && g_bOwned)
			SetConsoleTitleW(g_strTitle.c_str());
	}

	void SetVisible(bool bVisible)
	{
		std::scoped_lock lckConsole(g_mtxConsole);
		if (bVisible && !Create())
			return;
		if (g_hWindow && g_bOwned)
			ShowWindow(g_hWindow, bVisible ? SW_SHOW : SW_HIDE);
		g_bVisible = bVisible;
	}

	void Toggle()
	{
		std::scoped_lock lckConsole(g_mtxConsole);
		SetVisible(!g_bVisible);
	}

	bool IsVisible()
	{
		std::scoped_lock lckConsole(g_mtxConsole);
		return g_bVisible;
	}

	void Write(OmegaRenderHook::ELogLevel eLevel, std::string_view strLine)
	{
		std::scoped_lock lckConsole(g_mtxConsole);
		g_aBacklog.emplace_back(eLevel, std::string(strLine));
		if (g_aBacklog.size() > kBacklogLines)
			g_aBacklog.pop_front();
		WriteLine(eLevel, strLine);
	}

	void Shutdown()
	{
		std::scoped_lock lckConsole(g_mtxConsole);
		if (g_hOutput != INVALID_HANDLE_VALUE)
			CloseHandle(g_hOutput);
		g_hOutput = INVALID_HANDLE_VALUE;
		if (g_bOwned)
		{
			SetConsoleCtrlHandler(&CtrlHandler, FALSE);
			FreeConsole();
		}
		g_hWindow = nullptr;
		g_bOwned = false;
		g_bVisible = false;
	}
}
