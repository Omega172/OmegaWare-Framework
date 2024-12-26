#include "pch.h"

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

#if (SPOOF_THREAD_ADDRESS || SPOOF_RETURN_ADDRESSES)
	Utils::LogDebug(std::format("PrivilegedHandle: {:#010x}", reinterpret_cast<uintptr_t>(Memory::GetPrivilegedHandleToProcess())));
	if (!Memory::ResetTrampolineCollection())
		return false;
#endif

#if FRAMEWORK_UNREAL
	if (!FrameworkUnrealInit())
		return false;
#endif

#if FRAMEWORK_UNREAL
	Utils::LogDebug("Initializing FNames, this can take a bit.");
	Utils::LogDebug(std::format("Unreal: 0x{:x}", Framework::unreal.get()));
	FNames::Initialize();
	Utils::LogDebug("FNames initialized!");
#endif

	try {
		for (auto& pFeature : g_vecFeatures) {
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

DWORD __stdcall FrameworkMainThread(LPVOID lpParam)
{
	if (!FrameworkInit()) {
		Framework::bShouldRun = false;
		Utils::LogDebug("FrameworkInit() returned false, unloading...");
	}

	Framework::config = std::make_unique<Config>(); // Initalize the config class

	Framework::lua = std::make_unique<Lua>();

	Framework::bInitalized = true;
	Utils::LogDebug(std::format("{}: Initialized", Framework::Title));

	Framework::lua.get()->AutoRun();

	while (Framework::bShouldRun)
	{
		#if FRAMEWORK_UNREAL
			Framework::unreal.get()->RefreshActorList();
		#endif

		for (auto& pFeature : g_vecFeatures)
			pFeature->Run();

		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	Framework::console->SetVisibility(true); // Set the console to be visible when the cheat is unloading
	Utils::LogDebug(std::format("{}: Unloading...", Framework::Title)); // Log that the cheat is unloading

	delete Framework::lua.release();

	Framework::wndproc.get()->Destroy();
	Framework::renderer.get()->Destroy();

	// Destroy features
	for (auto& pFeature : g_vecFeatures)
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

#if SPOOF_THREAD_ADDRESS
	Memory::SpoofThreadAddress(FrameworkInit, hModule);
#else
	HANDLE hThread = CreateThread(nullptr, NULL, FrameworkMainThread, hModule, NULL, nullptr);
	if (hThread)
		CloseHandle(hThread);
#endif

	return TRUE;
}