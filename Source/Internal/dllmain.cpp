#include "pch.h"

#ifdef PROXY
#include "../Proxy/Proxy.hpp"
#endif

static bool FrameworkInit()
{
	char szModuleName[1024]{};
	if (!GetModuleFileNameA(Framework::hModule, szModuleName, sizeof(szModuleName)))
		return false;

	Framework::iModuleNameHash = CRC64::hash(szModuleName);

	if (!Framework::wndproc.get()->Setup())
		return false;

	if (!Framework::renderer.get()->Setup())
		return false;

#if ENGINE_UNREAL
	if (!FrameworkUnrealInit())
		return false;
#endif

#if ENGINE_UNREAL
	Utils::LogDebug("Initializing FNames, this can take a bit.");
	Utils::LogDebug(std::format("Unreal: {:#010x}", reinterpret_cast<uintptr_t>(Framework::unreal.get())));
#endif

	try {
		for (auto& pFeature : Framework::g_vecFeatures) {
			if (pFeature->SetupMenu() && pFeature->Setup())
				continue;

			Utils::LogError(std::format("Feature \"{}\" failed SetupMenu or Setup", pFeature->GetName()));
			return false;
		}
	}
	catch (const std::exception& e) {
		Utils::LogError(e.what());
		return false;
	}

	return true;
}

DWORD WINAPI FrameworkMainThread(LPVOID lpParam)
{
	if (!FrameworkInit()) {
		Framework::bShouldRun = false;
		Utils::LogDebug("FrameworkInit() returned false, unloading...");
	}

	Framework::config = std::make_unique<Config>(*Framework::menu);

	Framework::bInitalized = true;
	Utils::LogDebug(std::format("{}: Initialized", Framework::Title));

	while (Framework::bShouldRun)
	{
		for (auto& pFeature : Framework::g_vecFeatures)
			pFeature->Run();

		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	Framework::console->SetVisibility(true);
	Utils::LogDebug(std::format("{}: Unloading...", Framework::Title));

	Framework::wndproc.get()->Destroy();
	Framework::renderer.get()->Destroy();

	for (auto& pFeature : Framework::g_vecFeatures)
		pFeature->Destroy();

	std::this_thread::sleep_for(std::chrono::seconds(3));

	Framework::console->Destroy();

	FreeLibraryAndExitThread(Framework::hModule, EXIT_SUCCESS);
	return true;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ulReasonForCall, LPVOID lpReserved)
{
	DisableThreadLibraryCalls(hModule);
	Framework::hModule = hModule;

	if (ulReasonForCall != DLL_PROCESS_ATTACH)
		return TRUE;

#ifdef PROXY
	if (!ProxyInitRealVersionDll())
		return FALSE;
#endif

#ifdef _DEBUG
	Framework::console->SetVisibility(true);
#endif

	HANDLE hThread = CreateThread(NULL, 0, FrameworkMainThread, hModule, 0, NULL);
	if (hThread)
		CloseHandle(hThread);


	return TRUE;
}
