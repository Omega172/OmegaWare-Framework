#include <Windows.h>
#include <filesystem>
#include <memory>
#include <sstream>

#include "../OmegaWare Internal Framework/FrameworkConfig.h"
#include "Utils/Utils.h"
#include "Inject/Inject.h"
#include "GUI/GUI.h"

namespace Globals {
#ifdef _DEBUG
	std::unique_ptr<Console> console = std::make_unique<Console>(true, FRAMEWORK_CODENAME);
#endif
}

int __stdcall WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdLine, int cmdShow)
{
	LogDebugHere("Creating window...");
	GUI::CreateWnd(L"OmegaWare.xyz DLL Injector", L"OmegaWareWndClass");
	LogDebugHere("Window created.");

	LogDebugHere("Attempting to create D3D9 device...");
	if (!GUI::CreateDevice())
	{
		LogErrorHere("Failed to create D3D9 device.");
		return EXIT_FAILURE;
	}
	LogDebugHere("D3D9 device created.");

	LogDebugHere("Setting up ImGui...");
	GUI::CreateImGui();
	LogDebugHere("ImGui setup.");

	while (GUI::bKeepRunning)
	{
		GUI::BeginRender();
		GUI::Render();
		GUI::EndRender();
	}

	LogDebugHere("Performing cleanup...");
	GUI::DestroyImGui();
	GUI::DestroyDevice();
	GUI::DestroyWindow();
	LogDebugHere("Cleanup finished.");

#ifdef _DEBUG
	Globals::console.get()->Destroy();
#endif

	return EXIT_SUCCESS;
}