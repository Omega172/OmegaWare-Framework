#include "pch.h"
#include "Includes.hpp"

static bool FrameworkInit()
{
	char szModuleName[1024]{};
	if (!GetModuleFileNameA(Framework::hModule, szModuleName, sizeof(szModuleName)))
		return false;

	Framework::iModuleNameHash = CRC64::hash(szModuleName);

	// Initalize MinHook
	if (MH_Initialize() != MH_STATUS::MH_OK)
		return false;

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
	FNames::Initialize();
	Utils::LogDebug("FNames initialized!");
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

	Framework::config = std::make_unique<Config>(); // Initalize the config class

	Framework::bInitalized = true;
	Utils::LogDebug(std::format("{}: Initialized", Framework::Title));

	while (Framework::bShouldRun)
	{
#if ENGINE_UNREAL
		Framework::unreal.get()->RefreshActorList();
#endif

		for (auto& pFeature : Framework::g_vecFeatures)
			pFeature->Run();

		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	Framework::console->SetVisibility(true); // Set the console to be visible when the cheat is unloading
	Utils::LogDebug(std::format("{}: Unloading...", Framework::Title)); // Log that the cheat is unloading

	Framework::wndproc.get()->Destroy();
	Framework::renderer.get()->Destroy();

	// Destroy features
	for (auto& pFeature : Framework::g_vecFeatures)
		pFeature->Destroy();

	std::this_thread::sleep_for(std::chrono::seconds(3));

	Framework::console->Destroy();

	// Uninitialize MinHook, will also disable any lurking hooks still active as a safety net
	MH_Uninitialize();

	// Unload the module and exit the thread
	FreeLibraryAndExitThread(Framework::hModule, EXIT_SUCCESS);
	return true; // Return true if the initalization was successful
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ulReasonForCall, LPVOID lpReserved)
{
	DisableThreadLibraryCalls(hModule);
	Framework::hModule = hModule;

	if (ulReasonForCall != DLL_PROCESS_ATTACH)
		return TRUE;

#ifdef _DEBUG
	Framework::console->SetVisibility(true); // Set the console to be visible by default if the framework is in debug mode
#endif

	DisableThreadLibraryCalls(hModule);
	HANDLE hThread = CreateThread(NULL, 0, FrameworkMainThread, hModule, 0, NULL);
	if (hThread)
		CloseHandle(hThread);


	return TRUE;
}