#include "pch.h"

#include "Kiero/kiero.h"
#if FRAMEWORK_RENDER_D3D11
#include "Hooks/D3D11/D3D11Hooks.h"
#endif

#if FRAMEWORK_RENDER_D3D12
#include "Hooks/D3D12/D3D12Hooks.h"
#endif

namespace Cheat
{
	bool Init()
	{
	#if FRAMEWORK_RENDER_D3D11
		if (kiero::init(kiero::RenderType::D3D11) == kiero::Status::Success)
		{
			if (kiero::bind(8, reinterpret_cast<void**>(&oPresent), hkPresent) != kiero::Status::Success)
				return false;
		}
		else
			return false;
	#endif

	#if FRAMEWORK_UNREAL
		if (!CG::InitSdk())
			return false;

		if (!(*CG::UWorld::GWorld))
			std::cout << "Waiting for GWorld to initalize\n";

		while (!(*CG::UWorld::GWorld))
			continue;
	#endif

	#if FRAMEWORK_RENDER_D3D12
		if (kiero::init(kiero::RenderType::D3D12) == kiero::Status::Success)
		{
			if (kiero::bind(54, (void**)&oExecuteCommandLists, hkExecuteCommandLists) != kiero::Status::Success)
				return false;

			if (kiero::bind(58, (void**)&oSignal, hkSignal) != kiero::Status::Success)
				return false;

			if (kiero::bind(140, (void**)&oPresent, hkPresent) != kiero::Status::Success)
				return false;
			
			if (kiero::bind(84, (void**)&oDrawInstanced, hkDrawInstanced) != kiero::Status::Success)
				return false;

			if (kiero::bind(85, (void**)&oDrawIndexedInstanced, hkDrawIndexedInstanced) != kiero::Status::Success)
				return false;
		}
		else
			return false;
	#endif

		Utils::LogDebug(Utils::GetLocation(CurrentLoc), "Initalizing Globals");

	#if FRAMEWORK_UNREAL
		Utils::LogDebug(Utils::GetLocation(CurrentLoc), (std::stringstream() << "Unreal: 0x" << unreal.get()).str());
	#endif

		Utils::LogDebug(Utils::GetLocation(CurrentLoc), "Globals Initalized");

		// https://stackoverflow.com/questions/16711697/is-there-any-use-for-unique-ptr-with-array
		// Features
		//Features.push_back(std::make_unique<ESP>());

		for (size_t i = 0; i < Features.size(); i++)
		{
			Features[i].get()->Setup();
		}

		return true;
	}

	void HandleKeys()
	{
		if (GetAsyncKeyState(dwMenuKey) & 0x1)
		{
			GUI::bMenuOpen = !GUI::bMenuOpen;

			ImGui::GetIO().MouseDrawCursor = GUI::bMenuOpen;

			if (ImGui::GetIO().MouseDrawCursor)
				SetCursor(NULL);
		}

		if (GetAsyncKeyState(dwConsoleKey) & 0x1)
			console.get()->ToggleVisibility();

		if (GetAsyncKeyState(dwUnloadKey))
			bShouldRun = false;

		for (size_t i = 0; i < Features.size(); i++)
		{
			Features[i].get()->HandleKeys();
		}
	}

	DWORD __stdcall CheatThread(LPVOID lpParam)
	{
		hModule = reinterpret_cast<HMODULE>(lpParam);

#ifdef _DEBUG
		console.get()->SetVisibility(true);
#endif

		if (!Init())
		{
			Utils::LogError(Utils::GetLocation(CurrentLoc), Cheat::Title + ": Failed to initalize");
			bShouldRun = false;
		}
		else
			Utils::LogDebug(Utils::GetLocation(CurrentLoc), Cheat::Title + ": Initalized");

		while (bShouldRun)
		{
			HandleKeys();

			for (size_t i = 0; i < Features.size(); i++)
			{
				Features[i].get()->Run();
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}

		console.get()->SetVisibility(true);
		Utils::LogDebug(Utils::GetLocation(CurrentLoc), Cheat::Title + ": Unloading...");

		#if FRAMEWORK_RENDER_D3D12
		D3D12Release();
		kiero::shutdown();
		#endif

		// Destroy features
		for (size_t i = 0; i < Features.size(); i++)
		{
			Features[i].get()->Destroy();
		}

		console.get()->Destroy();

		std::this_thread::sleep_for(std::chrono::seconds(3));

		FreeLibraryAndExitThread(hModule, EXIT_SUCCESS);
		return TRUE;
	}
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ulReasonForCall, LPVOID lpReserved)
{
	DisableThreadLibraryCalls(hModule);

	if (ulReasonForCall != DLL_PROCESS_ATTACH)
		return TRUE;

	CreateThread(nullptr, NULL, Cheat::CheatThread, hModule, NULL, &Cheat::dwThreadID);

	return TRUE;
}
