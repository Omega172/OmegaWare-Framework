#include "pch.h"

DWORD __stdcall FrameworkInit(LPVOID lpParam)
{
	// Initalize MinHook
	if (MH_Initialize() != MH_STATUS::MH_OK)
		return false;

	if (!Framework::wndproc.get()->Setup())
		return false;

	if (!Framework::renderer.get()->Setup())
		return false;

	#if FRAMEWORK_UNREAL
		if (!FrameworkUnrealInit())
			return false;
	#endif

	LogDebugHere("Initalizing Globals, this can take a bit");

	#if FRAMEWORK_UNREAL
		LogDebugStreamHere("Unreal: 0x" << unreal.get());
		FNames::Initialize();
	#endif

	LogDebugHere("Globals Initalized"); // Log that the globals have been initalized

	HANDLE pPrivilagedHandle = Memory::GetPrivilegedHandleToProcess();
	if (pPrivilagedHandle)
		LogDebugStreamHere("PrivilagedHandle: 0x" << std::hex << pPrivilagedHandle);

	AddFeatures();

	try {
		for (size_t i = 0; i < Features.size(); i++) // A loop to grap the feature pointers and call their respective setup functions
		{
			bool bResult = Features[i]->Setup();
			if (!bResult)
			{
				LogErrorHere("Failed to setup feature: " + std::to_string(i));
				return false;
			}
		}
	}
	catch (char* e) {
		LogDebugHere(std::string(e));
	}

	Framework::config = std::make_unique<Config>(); // Initalize the config class

	Framework::hModule = reinterpret_cast<HMODULE>(lpParam);

	#ifdef _DEBUG
		Framework::console->SetVisibility(true); // Set the console to be visible if the framework is in debug mode
	#endif

	Framework::bInitalized = true;
	LogDebugHere(Framework::Title + ": Initalized");

	while (Framework::bShouldRun)
	{
		#if FRAMEWORK_UNREAL
			Framework::unreal.get()->RefreshActorList();
		#endif

		for (size_t i = 0; i < Features.size(); i++)
		{
			Features[i]->Run();
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	Framework::console->SetVisibility(true); // Set the console to be visible when the cheat is unloading
	LogDebugHere(Framework::Title + ": Unloading..."); // Log that the cheat is unloading

	Framework::wndproc.get()->Destroy();
	Framework::renderer.get()->Destroy();

	// Destroy features
	for (size_t i = 0; i < Features.size(); i++)
	{
		Features[i]->Destroy();
	}

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

	if (ulReasonForCall != DLL_PROCESS_ATTACH)
		return TRUE;

	HANDLE hThread = CreateThread(nullptr, NULL, FrameworkInit, hModule, NULL, nullptr);
	if (hThread) CloseHandle(hThread);

	return TRUE;
}