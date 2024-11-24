#include "pch.h"
#if FRAMEWORK_RENDER_D3D11 || FRAMEWORK_RENDER_DYNAMIC

static ID3D11Device* g_pDevice = NULL;
static ID3D11DeviceContext* g_pDeviceContext = NULL;
static ID3D11RenderTargetView* g_pRenderTargetView = NULL;
static IDXGISwapChain* g_pSwapChain = NULL;

static DXGI_FORMAT GetCorrectDXGIFormat(DXGI_FORMAT currentFormat) {
	switch (currentFormat) {
	case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
		return DXGI_FORMAT_R8G8B8A8_UNORM;
	}

	return currentFormat;
}

static bool CreateDevice(HWND hWnd) {

	DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
	swapChainDesc.Windowed = TRUE;
	swapChainDesc.BufferCount = 2;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.OutputWindow = hWnd;
	swapChainDesc.SampleDesc.Count = 1;

	constexpr D3D_FEATURE_LEVEL featureLevels[] = {
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_0,
	};

	HRESULT hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_NULL, NULL, 0, featureLevels, 2, D3D11_SDK_VERSION, &swapChainDesc, &g_pSwapChain, &g_pDevice, nullptr, nullptr);

	return hr == S_OK;
}

static void CreateRenderTarget(IDXGISwapChain* pSwapChain)
{
	ID3D11Texture2D* pBackBuffer = NULL;
	pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
	if (pBackBuffer) {
		DXGI_SWAP_CHAIN_DESC sd;
		pSwapChain->GetDesc(&sd);

		D3D11_RENDER_TARGET_VIEW_DESC desc = {};
		desc.Format = GetCorrectDXGIFormat(sd.BufferDesc.Format);

		if (!IsValidObjectPtr(g_pDevice)) {
			LogErrorHere("This is not funny!");
			return;
		}

		desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
		if (FAILED(g_pDevice->CreateRenderTargetView(pBackBuffer, &desc, &g_pRenderTargetView))) {
			desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

			if (FAILED(g_pDevice->CreateRenderTargetView(pBackBuffer, &desc, &g_pRenderTargetView))) {

				if (FAILED(g_pDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_pRenderTargetView))) {
					LogErrorHere("CreateRenderTargetView failure!");
				}
			}
		}

		pBackBuffer->Release();
	}
}

static void CleanupRenderTarget() {
	LogDebugHere("DX11 CleanupRenderTarget()");

	if (g_pRenderTargetView) {
		g_pRenderTargetView->Release();
		g_pRenderTargetView = NULL;
	}
}

static void CleanupDevice() {
	LogDebugHere("DX11 CleanupDevice()");

	CleanupRenderTarget();

	if (g_pSwapChain) {
		g_pSwapChain->Release();
		g_pSwapChain = NULL;
	}
	if (g_pDevice) {
		g_pDevice->Release();
		g_pDevice = NULL;
	}
	if (g_pDeviceContext) {
		g_pDeviceContext->Release();
		g_pDeviceContext = NULL;
	}
}

static void RenderImGui(IDXGISwapChain* pSwapChain) {
	if (!ImGui::GetCurrentContext())
		return;

	if (!ImGui::GetIO().BackendRendererUserData) {

		if (!SUCCEEDED(pSwapChain->GetDevice(IID_PPV_ARGS(&g_pDevice)))) {
			LogErrorHere("Couldnt get device ?!???");
			return;
		}

		g_pDevice->GetImmediateContext(&g_pDeviceContext);
		ImGui_ImplDX11_Init(g_pDevice, g_pDeviceContext);
	}

	if (!g_pRenderTargetView) {
		CreateRenderTarget(pSwapChain);
		return;
	}


	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	ImGui::PushFont(CurrentFont);
	GUI::Render();
	ImGui::PopFont();

	ImGui::Render();
	g_pDeviceContext->OMSetRenderTargets(1, &g_pRenderTargetView, NULL);
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

static Memory::Hook<HRESULT(WINAPI*)(IDXGISwapChain*, UINT, UINT)> oPresent;
static HRESULT WINAPI hkPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags) {
	RenderImGui(pSwapChain);

	return oPresent(pSwapChain, SyncInterval, Flags);
}

static Memory::Hook<HRESULT(WINAPI*)(IDXGISwapChain*, UINT, UINT, const DXGI_PRESENT_PARAMETERS*)> oPresent1;
static HRESULT WINAPI hkPresent1(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT PresentFlags, const DXGI_PRESENT_PARAMETERS* pPresentParameters) {
	RenderImGui(pSwapChain);

	return oPresent1(pSwapChain, SyncInterval, PresentFlags, pPresentParameters);
}

static Memory::Hook<HRESULT(WINAPI*)(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT)> oResizeBuffers;
static HRESULT WINAPI hkResizeBuffers(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags) {
	CleanupRenderTarget();

	return oResizeBuffers(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);
}

static Memory::Hook<HRESULT(WINAPI*)(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT, const UINT*, IUnknown* const*)> oResizeBuffers1;
static HRESULT WINAPI hkResizeBuffers1(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags, const UINT* pCreationNodeMask, IUnknown* const* ppPresentQueue) {
	CleanupRenderTarget();

	return oResizeBuffers1(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags, pCreationNodeMask, ppPresentQueue);
}

static Memory::Hook<HRESULT(WINAPI*)(IDXGIFactory*, IUnknown*, DXGI_SWAP_CHAIN_DESC*, IDXGISwapChain**)> oCreateSwapChain;
static HRESULT WINAPI hkCreateSwapChain(IDXGIFactory* pFactory, IUnknown* pDevice, DXGI_SWAP_CHAIN_DESC* pDesc, IDXGISwapChain** ppSwapChain) {
	CleanupRenderTarget();

	return oCreateSwapChain(pFactory, pDevice, pDesc, ppSwapChain);
}

static Memory::Hook<HRESULT(WINAPI*)(IDXGIFactory*, IUnknown*, HWND, const DXGI_SWAP_CHAIN_DESC1*, const DXGI_SWAP_CHAIN_FULLSCREEN_DESC*, IDXGIOutput*, IDXGISwapChain1**)> oCreateSwapChainForHwnd;
static HRESULT WINAPI hkCreateSwapChainForHwnd(IDXGIFactory* pFactory, IUnknown* pDevice, HWND hWnd, const DXGI_SWAP_CHAIN_DESC1* pDesc, const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* pFullscreenDesc, IDXGIOutput* pRestrictToOutput, IDXGISwapChain1** ppSwapChain) {
	CleanupRenderTarget();

	return oCreateSwapChainForHwnd(pFactory, pDevice, hWnd, pDesc, pFullscreenDesc, pRestrictToOutput, ppSwapChain);
}

static Memory::Hook<HRESULT(WINAPI*)(IDXGIFactory*, IUnknown*, IUnknown*, const DXGI_SWAP_CHAIN_DESC1*, IDXGIOutput*, IDXGISwapChain1**)> oCreateSwapChainForCoreWindow;
static HRESULT WINAPI hkCreateSwapChainForCoreWindow(IDXGIFactory* pFactory, IUnknown* pDevice, IUnknown* pWindow, const DXGI_SWAP_CHAIN_DESC1* pDesc, IDXGIOutput* pRestrictToOutput, IDXGISwapChain1** ppSwapChain) {
	CleanupRenderTarget();

	return oCreateSwapChainForCoreWindow(pFactory, pDevice, pWindow, pDesc, pRestrictToOutput, ppSwapChain);
}

static Memory::Hook<HRESULT(WINAPI*)(IDXGIFactory*, IUnknown*, const DXGI_SWAP_CHAIN_DESC1*, IDXGIOutput*, IDXGISwapChain1**)> oCreateSwapChainForComposition;
static HRESULT WINAPI hkCreateSwapChainForComposition(IDXGIFactory* pFactory, IUnknown* pDevice, const DXGI_SWAP_CHAIN_DESC1* pDesc, IDXGIOutput* pRestrictToOutput, IDXGISwapChain1** ppSwapChain) {
	CleanupRenderTarget();

	return oCreateSwapChainForComposition(pFactory, pDevice, pDesc, pRestrictToOutput, ppSwapChain);
}

bool RendererHooks::D3D11Setup()
{
	if (!CreateDevice(Framework::wndproc.get()->hwndWindow)) {
		LogErrorHere("Unable to create dx11 device!");
		return false;
	}

	IDXGIDevice* pDXGIDevice = NULL;
	g_pDevice->QueryInterface(IID_PPV_ARGS(&pDXGIDevice));

	IDXGIAdapter* pDXGIAdapter = NULL;
	pDXGIDevice->GetAdapter(&pDXGIAdapter);

	IDXGIFactory* pIDXGIFactory = NULL;
	pDXGIAdapter->GetParent(IID_PPV_ARGS(&pIDXGIFactory));

	// IDXGIFactory
	if (oCreateSwapChain.HookVirtualMethod(hkCreateSwapChain, pIDXGIFactory, 10) != MH_OK) {
		LogErrorHere("CreateSwapCain hook couldnt be created!");
		return false;
	}

	if (oCreateSwapChainForHwnd.HookVirtualMethod(hkCreateSwapChainForHwnd, pIDXGIFactory, 15) != MH_OK) {
		LogErrorHere("CreateSwapChainForHwnd hook couldnt be created!");
		return false;
	}

	if (oCreateSwapChainForCoreWindow.HookVirtualMethod(hkCreateSwapChainForCoreWindow, pIDXGIFactory, 16) != MH_OK) {
		LogErrorHere("CreateSwapChainForCoreWindow hook couldnt be created!");
		return false;
	}

	if (oCreateSwapChainForComposition.HookVirtualMethod(hkCreateSwapChainForComposition, pIDXGIFactory, 24) != MH_OK) {
		LogErrorHere("CreateSwapChainForComposition hook couldnt be created!");
		return false;
	}
	// IDXGIFactory

	// SwapChain
	if (oPresent.HookVirtualMethod(hkPresent, g_pSwapChain, 8) != MH_OK) {
		LogErrorHere("Present hook couldnt be created!");
		return false;
	}

	if (oResizeBuffers.HookVirtualMethod(hkResizeBuffers, g_pSwapChain, 13) != MH_OK) {
		LogErrorHere("ResizeBuffers hook couldnt be created!");
		return false;
	}

	if (oPresent1.HookVirtualMethod(hkPresent1, g_pSwapChain, 22) != MH_OK) {
		LogErrorHere("Present1 hook couldnt be created!");
		return false;
	}

	if (oResizeBuffers1.HookVirtualMethod(hkResizeBuffers1, g_pSwapChain, 39) != MH_OK) {
		LogErrorHere("ResizeBuffers1 hook couldnt be created!");
		return false;
	}
	// SwapChain

	pIDXGIFactory->Release();
	pDXGIAdapter->Release();
	pDXGIDevice->Release();

	CleanupRenderTarget();

	return true;
}

void RendererHooks::D3D11Destroy()
{
	oPresent.RemoveHook();
	oPresent1.RemoveHook();
	oResizeBuffers1.RemoveHook();
	oCreateSwapChain.RemoveHook();
	oCreateSwapChainForHwnd.RemoveHook();
	oCreateSwapChainForCoreWindow.RemoveHook();
	oCreateSwapChainForComposition.RemoveHook();

	if (ImGui::GetCurrentContext()) {
		ImGuiIO& io = ImGui::GetIO();

		if (io.BackendRendererUserData)
			ImGui_ImplDX11_Shutdown();

		if (io.BackendPlatformUserData)
			ImGui_ImplWin32_Shutdown();

		ImGui::DestroyContext();
	}

	CleanupDevice();
}

#endif