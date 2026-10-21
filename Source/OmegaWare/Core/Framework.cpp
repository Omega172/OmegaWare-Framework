#include "Core/Framework.hpp"
#include "Config/Config.hpp"
#include "Core/Console.hpp"
#include "Core/CrashDump.hpp"
#include "Core/Log.hpp"
#include "Core/ReadyWatch.hpp"
#include "Engines/Engine.hpp"
#include "Features/Feature.hpp"
#include "Hooks/Hook.hpp"
#include "Menu/Menu.hpp"
#include "Overlay/Overlay.hpp"
#include "Plugins/Plugins.hpp"

#include "OmegaRenderHook/OmegaRenderHook.hpp"

#include <ShlObj.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <thread>

#ifndef OMEGAWARE_GAME_DEPTH
#define OMEGAWARE_GAME_DEPTH 1
#endif

namespace OmegaWare
{
	namespace
	{
		HMODULE g_hModule = nullptr;
		std::atomic<bool> g_bUnloadRequested = false;
		clOverlay g_clOverlay;

		[[nodiscard]] std::string ProcessName()
		{
			wchar_t szPath[MAX_PATH]{};
			GetModuleFileNameW(nullptr, szPath, MAX_PATH);
			const std::u8string strName = std::filesystem::path(szPath).filename().u8string();
			return { strName.begin(), strName.end() };
		}

		[[nodiscard]] bool IsProcessForeground()
		{
			DWORD uProcessId = 0;
			GetWindowThreadProcessId(GetForegroundWindow(), &uProcessId);
			return uProcessId == GetCurrentProcessId();
		}

		bool WaitForHostGraphics()
		{
			LogInfo("waiting for the game to render before hooking");
			const ReadyWatch::EResult eResult = ReadyWatch::Wait(std::chrono::minutes(5));
			ReadyWatch::Stop();
			switch (eResult)
			{
			case ReadyWatch::EResult::Rendering:
				LogInfo("the game is rendering ({}, {} frames presented), hooking", ReadyWatch::ApiName(), ReadyWatch::Presents());
				return true;
			case ReadyWatch::EResult::Fallback:
				LogWarning("no presented frames were seen, but the game has a window and a graphics device, hooking anyway");
				return true;
			case ReadyWatch::EResult::TimedOut:
				LogWarning("the game never started rendering, not hooking");
				return false;
			default:
				return false;
			}
		}

		void Unload()
		{
			LogInfo("{} unloading", kTitle);
			const bool bPipelineRemoved = OmegaRenderHook::Pipeline::Uninstall();
			const bool bSafe = OmegaRenderHook::InputBlock::Uninstall() && bPipelineRemoved;
			Plugins::Shutdown();
			Features::DestroyAll();
			if (!Hooks::WaitForCalls(std::chrono::seconds(1)))
				LogWarning("hook calls were still in flight after features were destroyed");
			g_clOverlay.Shutdown();
			Engine::Shutdown();
#if OMEGAWARE_PROXY
			LogInfo("{}", bSafe ? "unloaded, the proxy stays loaded so the game keeps its " OMEGAWARE_PROXY_DLL " exports" : "hooks could not be fully removed");
#else
			LogInfo("{}", bSafe ? "unloaded" : "hooks could not be fully removed, staying loaded");
#endif
			Menu::Shutdown();
			Console::Shutdown();
			CrashDump::Uninstall();
			CloseLog();
#if OMEGAWARE_PROXY
			static_cast<void>(bSafe);
#else
			if (bSafe)
				FreeLibraryAndExitThread(g_hModule, 0);
#endif
		}

		DWORD WINAPI MainThread(LPVOID)
		{
			OpenLog(DataDirectory() / "Logs");
			if (OMEGAWARE_PROXY)
				ReadyWatch::Start();
			CrashDump::Install(DataDirectory() / "Crashes");
			Menu::Initialize();
			Console::SetTitle(std::format("{} | {}", kTitle, ProcessName()));
#ifdef _DEBUG
			Console::SetVisible(true);
#endif
			LogInfo("{} {} for {} loaded into {} (pid {}) {}", kTitle, kVersion, kGame, ProcessName(), GetCurrentProcessId(), OMEGAWARE_PROXY ? "as a " OMEGAWARE_PROXY_DLL " proxy" : "by injection");

			try
			{
				Config::Initialize(DataDirectory() / "Configs");
				LogInfo("config profile \"{}\" active", Config::GetCurrent());
				const Menu::stHotkeys stKeys = Menu::Hotkeys();
				LogInfo("{} toggles the menu, {} toggles the console, {} unloads", Menu::HotkeyName(stKeys.iMenu), Menu::HotkeyName(stKeys.iConsole), Menu::HotkeyName(stKeys.iUnload));
			}
			catch (const std::exception& rException)
			{
				LogError("config failed to initialize: {}", rException.what());
			}

			if (OMEGAWARE_PROXY && !WaitForHostGraphics())
			{
				Unload();
				return 0;
			}

			OmegaRenderHook::stPipelineConfig stConfig;
			stConfig.pListener = &g_clOverlay;
			stConfig.bGameDepth = OMEGAWARE_GAME_DEPTH != 0;
			if (auto expInstalled = OmegaRenderHook::Pipeline::Install(stConfig); !expInstalled)
			{
				LogError("pipeline install failed: {}", expInstalled.error());
				Unload();
				return 0;
			}

			OmegaRenderHook::InputBlock::Install();
			Engine::Initialize([] { return g_clOverlay.IsAttached(); });
			Features::SetupAll();
			Plugins::Initialize(DataDirectory() / "Plugins", [] { return g_clOverlay.LockGui(); });

			auto tpNextRun = std::chrono::steady_clock::now();
			while (!IsUnloadRequested())
			{
				if (const int iUnload = Menu::Hotkeys().iUnload; iUnload > 0 && !g_clOverlay.IsAttached() && IsProcessForeground() && (GetAsyncKeyState(iUnload) & 0x8000))
					RequestUnload();

				if (std::chrono::steady_clock::now() >= tpNextRun)
				{
					Engine::Update();
					Plugins::Update();
					Features::RunAll();
					tpNextRun += std::chrono::milliseconds(100);
					if (tpNextRun < std::chrono::steady_clock::now())
						tpNextRun = std::chrono::steady_clock::now() + std::chrono::milliseconds(100);
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
			}

			Unload();
			return 0;
		}
	}

	void Start(HMODULE hModule)
	{
		g_hModule = hModule;
		wchar_t szProcess[MAX_PATH]{};
		GetModuleFileNameW(nullptr, szProcess, MAX_PATH);
		const std::wstring strProcess = std::filesystem::path(szProcess).filename().wstring();
		if (!kProcess.empty() && _wcsicmp(std::filesystem::path(kProcess).wstring().c_str(), strProcess.c_str()) != 0)
			return;
#if OMEGAWARE_PROXY
		constexpr std::array<std::wstring_view, 3> aHelperProcesses = { L"UnityCrashHandler32.exe", L"UnityCrashHandler64.exe", L"CrashReportClient.exe" };
		if (std::ranges::any_of(aHelperProcesses, [&](std::wstring_view strHelper) { return _wcsicmp(strHelper.data(), strProcess.c_str()) == 0; }))
			return;
		HMODULE hPinned = nullptr;
		GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN, reinterpret_cast<LPCWSTR>(&Start), &hPinned);
		ReadyWatch::Start();
#endif
		if (HANDLE hThread = CreateThread(nullptr, 0, &MainThread, nullptr, 0, nullptr))
			CloseHandle(hThread);
	}

	void RequestUnload()
	{
		g_bUnloadRequested = true;
	}

	bool IsUnloadRequested()
	{
		return g_bUnloadRequested;
	}

	std::filesystem::path DataDirectory()
	{
		static const std::filesystem::path s_pathData = []
		{
			wchar_t szExe[MAX_PATH]{};
			GetModuleFileNameW(nullptr, szExe, MAX_PATH);
			const std::filesystem::path pathExe(szExe);
			wchar_t szDocuments[MAX_PATH]{};
			if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_PERSONAL | CSIDL_FLAG_CREATE, nullptr, SHGFP_TYPE_CURRENT, szDocuments)))
				return std::filesystem::path(szDocuments) / std::filesystem::path(kName) / pathExe.stem();
			wchar_t szModule[MAX_PATH]{};
			GetModuleFileNameW(g_hModule, szModule, MAX_PATH);
			return std::filesystem::path(szModule).parent_path() / std::filesystem::path(kName);
		}();
		return s_pathData;
	}
}
