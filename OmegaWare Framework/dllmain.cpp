#include "pch.h"

#define DO_THREAD_SLEEP 1
#define THREAD_SLEEP_TIME 100

namespace Cheat
{
	bool Init()
	{
		if (MH_Initialize() != MH_STATUS::MH_OK)
			return false;

		if (!wndproc.get()->Setup())
			return false;

		if (!renderer.get()->Setup())
			return false;

	#if FRAMEWORK_UNREAL // If the framework is Unreal initalize the SDK assuming the SDK was generated with CheatGeat by Cormm
		if (!CG::InitSdk())
			return false;

		if (!(*CG::UWorld::GWorld))
			Utils::LogError(Utils::GetLocation(CurrentLoc), "Waiting for GWorld to initalize");

		while (!(*CG::UWorld::GWorld))
			continue;
	#endif

		Utils::LogDebug(Utils::GetLocation(CurrentLoc), "Initalizing Globals, this can take a bit"); // Log that the globals are being initalized

	#if FRAMEWORK_UNREAL // If using the Unreal framework print the pointer to the Unreal class to make sure it was initalized
		Utils::LogDebug(Utils::GetLocation(CurrentLoc), (std::stringstream() << "Unreal: 0x" << unreal.get()).str());
		FNames::Initialize();
	#endif

		// Add other globals that need to be initalized here

		Utils::LogDebug(Utils::GetLocation(CurrentLoc), "Globals Initalized"); // Log that the globals have been initalized

		HANDLE pPrivilagedHandle = Memory::GetPrivilegedHandleToProcess();
		if (pPrivilagedHandle)
			Utils::LogDebug(Utils::GetLocation(CurrentLoc), (std::stringstream() << "PrivilagedHandle: 0x" << std::hex << pPrivilagedHandle).str());


		localization = std::make_unique<Localization>();
		if (!localization->IsInitialized())
			return false;

		// https://stackoverflow.com/questions/16711697/is-there-any-use-for-unique-ptr-with-array
		// Features
		Features.push_back(std::make_unique<ExampleFeature>());

		try {
			for (size_t i = 0; i < Features.size(); i++) // A loop to grap the feature pointers and call their respective setup functions
			{
				bool bResult = Features[i]->Setup();
				if (!bResult)
				{
					Utils::LogError(Utils::GetLocation(CurrentLoc), "Failed to setup feature: " + std::to_string(i));
					return false;
				}
			}
		} catch (char* e) {
			Utils::LogDebug(Utils::GetLocation(CurrentLoc), std::string(e));
		}

		config = std::make_unique<Config>(); // Initalize the config class

		return true; // Return true if the initalization was successful
	}

	DWORD __stdcall CheatThread(LPVOID lpParam)
	{
		hModule = reinterpret_cast<HMODULE>(lpParam); // Store the module handle which is used for unloading the module

#ifdef _DEBUG
		console->SetVisibility(true); // Set the console to be visible if the framework is in debug mode
#endif

		if (!Init())
		{
			// If the initalization failed log an error and set the boolean to false to stop the cheat from running
			Utils::LogError(Utils::GetLocation(CurrentLoc), Cheat::Title + ": Failed to initalize");
			bShouldRun = false;
		}
		else
		{
			// If the initalization was successful log that the cheat was initalized
			Cheat::bInitalized = true;
			Utils::LogDebug(Utils::GetLocation(CurrentLoc), Cheat::Title + ": Initalized");
		}

		while (bShouldRun) // the main process loop used to asynchonously run the features and handle the keys independently from the game
		{
#if FRAMEWORK_UNREAL
			Cheat::unreal.get()->RefreshActorList();
#endif

			for (size_t i = 0; i < Features.size(); i++)
			{
				Features[i]->Run();
			}

// If the thread sleep is enabled sleep for the specified amount of time
// This is used to reduce the CPU usage of the module, I would recommend keeping this enabled but added the option to disable it if needed for testing and when messing with less CPU intensive games
#if DO_THREAD_SLEEP
			std::this_thread::sleep_for(std::chrono::milliseconds(THREAD_SLEEP_TIME));
#endif
		}
		console->SetVisibility(true); // Set the console to be visible when the cheat is unloading
		Utils::LogDebug(Utils::GetLocation(CurrentLoc), Cheat::Title + ": Unloading..."); // Log that the cheat is unloading

		wndproc.get()->Destroy();
		renderer.get()->Destroy();
		
		// Destroy features
		for (size_t i = 0; i < Features.size(); i++)
		{
			Features[i]->Destroy();
		}

		std::this_thread::sleep_for(std::chrono::seconds(3));

		console->Destroy();

		// Uninitialize MinHook, will also disable any lurking hooks still active as a safety net
		MH_Uninitialize(); 

		// Unload the module and exit the thread
		FreeLibraryAndExitThread(hModule, EXIT_SUCCESS); 
		return TRUE;
	}
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ulReasonForCall, LPVOID lpReserved)
{
	// Disable unwanted and unneeded thread calls
	DisableThreadLibraryCalls(hModule); 

	if (ulReasonForCall != DLL_PROCESS_ATTACH)
		return TRUE;

	HANDLE hThread = CreateThread(nullptr, NULL, Cheat::CheatThread, hModule, NULL, &Cheat::dwThreadID);
	if (hThread)
		CloseHandle(hThread);

	return TRUE;
}
