#include "pch.h"
#if FRAMEWORK_RENDER_D3D11

bool D3D11Setup()
{
	if (!CreateDevice(Cheat::wndproc.get()->hwndWindow)) {
		Utils::LogError(Utils::GetLocation(CurrentLoc), "Unable to create dx11 device!");
		return false;
	}

	IDXGIDevice* pDXGIDevice = NULL;
	g_pDevice->QueryInterface(IID_PPV_ARGS(&pDXGIDevice));

	IDXGIAdapter* pDXGIAdapter = NULL;
	pDXGIDevice->GetAdapter(&pDXGIAdapter);

	IDXGIFactory* pIDXGIFactory = NULL;
	pDXGIAdapter->GetParent(IID_PPV_ARGS(&pIDXGIFactory));

	// CreateSwapChain
	void* pMethod = Utils::GetVirtualMethod(pIDXGIFactory, 10);
	if (!pMethod) {
		Utils::LogError(Utils::GetLocation(CurrentLoc), "CreateSwapChain was null!");
		return false;
	}
	if (MH_CreateHook(pMethod, hkCreateSwapChain, reinterpret_cast<void**>(&oCreateSwapChain)) != MH_OK || MH_EnableHook(pMethod) != MH_OK) {
		Utils::LogError(Utils::GetLocation(CurrentLoc), "CreateSwapChain hook couldnt be created!");
		return false;
	}
	// CreateSwapChain

	// CreateSwapChainForHwnd
	pMethod = Utils::GetVirtualMethod(pIDXGIFactory, 15);
	if (!pMethod) {
		Utils::LogError(Utils::GetLocation(CurrentLoc), "CreateSwapChainForHwnd was null!");
		return false;
	}
	if (MH_CreateHook(pMethod, hkCreateSwapChainForHwnd, reinterpret_cast<void**>(&oCreateSwapChainForHwnd)) != MH_OK || MH_EnableHook(pMethod) != MH_OK) {
		Utils::LogError(Utils::GetLocation(CurrentLoc), "CreateSwapChainForHwnd hook couldnt be created!");
		return false;
	}
	// CreateSwapChainForHwnd

	// CreateSwapChainForCoreWindow
	pMethod = Utils::GetVirtualMethod(pIDXGIFactory, 16);
	if (!pMethod) {
		Utils::LogError(Utils::GetLocation(CurrentLoc), "CreateSwapChainForCoreWindow was null!");
		return false;
	}
	if (MH_CreateHook(pMethod, hkCreateSwapChainForCoreWindow, reinterpret_cast<void**>(&oCreateSwapChainForCoreWindow)) != MH_OK || MH_EnableHook(pMethod) != MH_OK) {
		Utils::LogError(Utils::GetLocation(CurrentLoc), "CreateSwapChainForCoreWindow hook couldnt be created!");
		return false;
	}
	// CreateSwapChainForCoreWindow

	// CreateSwapChainForComposition
	pMethod = Utils::GetVirtualMethod(pIDXGIFactory, 24);
	if (!pMethod) {
		Utils::LogError(Utils::GetLocation(CurrentLoc), "CreateSwapChainForComposition was null!");
		return false;
	}
	if (MH_CreateHook(pMethod, hkCreateSwapChainForComposition, reinterpret_cast<void**>(&oCreateSwapChainForComposition)) != MH_OK || MH_EnableHook(pMethod) != MH_OK) {
		Utils::LogError(Utils::GetLocation(CurrentLoc), "CreateSwapChainForComposition hook couldnt be created!");
		return false;
	}
	// CreateSwapChainForComposition

	// Present
	pMethod = Utils::GetVirtualMethod(g_pSwapChain, 8);
	if (!pMethod) {
		Utils::LogError(Utils::GetLocation(CurrentLoc), "Present was null!");
		return false;
	}
	if (MH_CreateHook(pMethod, hkPresent, reinterpret_cast<void**>(&oPresent)) != MH_OK || MH_EnableHook(pMethod) != MH_OK) {
		Utils::LogError(Utils::GetLocation(CurrentLoc), "Present hook couldnt be created!");
		return false;
	}
	// Present

	// ResizeBuffers
	pMethod = Utils::GetVirtualMethod(g_pSwapChain, 13);
	if (!pMethod) {
		Utils::LogError(Utils::GetLocation(CurrentLoc), "ResizeBuffers was null!");
		return false;
	}
	if (MH_CreateHook(pMethod, hkResizeBuffers, reinterpret_cast<void**>(&oResizeBuffers)) != MH_OK || MH_EnableHook(pMethod) != MH_OK) {
		Utils::LogError(Utils::GetLocation(CurrentLoc), "ResizeBuffers hook couldnt be created!");
		return false;
	}
	// ResizeBuffers

	// Present1
	pMethod = Utils::GetVirtualMethod(g_pSwapChain, 22);
	if (!pMethod) {
		Utils::LogError(Utils::GetLocation(CurrentLoc), "Present1 was null!");
		return false;
	}
	if (MH_CreateHook(pMethod, hkPresent1, reinterpret_cast<void**>(&oPresent1)) != MH_OK || MH_EnableHook(pMethod) != MH_OK) {
		Utils::LogError(Utils::GetLocation(CurrentLoc), "Present1 hook couldnt be created!");
		return false;
	}
	// Present1

	// ResizeBuffers1
	pMethod = Utils::GetVirtualMethod(g_pSwapChain, 39);
	if (!pMethod) {
		Utils::LogError(Utils::GetLocation(CurrentLoc), "ResizeBuffers1 was null!");
		return false;
	}
	if (MH_CreateHook(pMethod, hkResizeBuffers1, reinterpret_cast<void**>(&oResizeBuffers1)) != MH_OK || MH_EnableHook(pMethod) != MH_OK) {
		Utils::LogError(Utils::GetLocation(CurrentLoc), "ResizeBuffers1 hook couldnt be created!");
		return false;
	}
	// ResizeBuffers1

	pIDXGIFactory->Release();
	pDXGIAdapter->Release();
	pDXGIDevice->Release();

	CleanupRenderTarget();

	return true;
}

void D3D11Destroy()
{

	if (ImGui::GetCurrentContext()) {
		ImGuiIO& io = ImGui::GetIO();

		if (io.BackendRendererUserData)
			ImGui_ImplDX11_Shutdown();

		if (io.BackendPlatformUserData)
			ImGui_ImplWin32_Shutdown();

		ImGui::DestroyContext();
	}

	CleanupDevice();

	MH_DisableHook(MH_ALL_HOOKS);

	// you need to MH_RemoveHook for each hook you have created, otherwise it will cause a crash
}

#endif