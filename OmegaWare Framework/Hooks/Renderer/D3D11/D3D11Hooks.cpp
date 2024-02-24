#include "pch.h"
#if FRAMEWORK_RENDER_D3D11

typedef LRESULT(CALLBACK* WNDPROC)(HWND, UINT, WPARAM, LPARAM);
typedef HRESULT(WINAPI* Present)(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
typedef HRESULT(WINAPI* Present1)(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT PresentFlags, const DXGI_PRESENT_PARAMETERS* pPresentParameters);
typedef HRESULT(WINAPI* ResizeBuffers)(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);
typedef HRESULT(WINAPI* ResizeBuffers1)(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags, const UINT* pCreationNodeMask, IUnknown* const* ppPresentQueue);
typedef HRESULT(WINAPI* CreateSwapChain)(IDXGIFactory* pFactory, IUnknown* pDevice, DXGI_SWAP_CHAIN_DESC* pDesc, IDXGISwapChain** ppSwapChain);
typedef HRESULT(WINAPI* CreateSwapChainForHwnd)(IDXGIFactory* pFactory, IUnknown* pDevice, HWND hWnd, const DXGI_SWAP_CHAIN_DESC1* pDesc, const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* pFullscreenDesc, IDXGIOutput* pRestrictToOutput, IDXGISwapChain1** ppSwapChain);
typedef HRESULT(WINAPI* CreateSwapChainForCoreWindow)(IDXGIFactory* pFactory, IUnknown* pDevice, IUnknown* pWindow, const DXGI_SWAP_CHAIN_DESC1* pDesc, IDXGIOutput* pRestrictToOutput, IDXGISwapChain1** ppSwapChain);
typedef HRESULT(WINAPI* CreateSwapChainForComposition)(IDXGIFactory* pFactory, IUnknown* pDevice, const DXGI_SWAP_CHAIN_DESC1* pDesc, IDXGIOutput* pRestrictToOutput, IDXGISwapChain1** ppSwapChain);

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
			Utils::LogError(Utils::GetLocation(CurrentLoc), "This is not funny!");
			return;
		}

		desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
		if (FAILED(g_pDevice->CreateRenderTargetView(pBackBuffer, &desc, &g_pRenderTargetView))) {
			desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

			if (FAILED(g_pDevice->CreateRenderTargetView(pBackBuffer, &desc, &g_pRenderTargetView))) {

				if (FAILED(g_pDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_pRenderTargetView))) {
					Utils::LogError(Utils::GetLocation(CurrentLoc), "CreateRenderTargetView failure!");
				}
			}
		}

		pBackBuffer->Release();
	}
}

static void CleanupRenderTarget() {
	Utils::LogDebug(Utils::GetLocation(CurrentLoc), "DX11 CleanupRenderTarget()");

	if (g_pRenderTargetView) {
		g_pRenderTargetView->Release();
		g_pRenderTargetView = NULL;
	}
}

static void CleanupDevice() {
	Utils::LogDebug(Utils::GetLocation(CurrentLoc), "DX11 CleanupDevice()");

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
			Utils::LogError(Utils::GetLocation(CurrentLoc), "Couldnt get device?!???");
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

static Present oPresent;
static HRESULT WINAPI hkPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags) {
	RenderImGui(pSwapChain);

	return oPresent(pSwapChain, SyncInterval, Flags);
}

static Present1 oPresent1;
static HRESULT WINAPI hkPresent1(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT PresentFlags, const DXGI_PRESENT_PARAMETERS* pPresentParameters) {
	RenderImGui(pSwapChain);

	return oPresent1(pSwapChain, SyncInterval, PresentFlags, pPresentParameters);
}

static ResizeBuffers oResizeBuffers;
static HRESULT WINAPI hkResizeBuffers(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags) {
	CleanupRenderTarget();

	return oResizeBuffers(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);
}

static ResizeBuffers1 oResizeBuffers1;
static HRESULT WINAPI hkResizeBuffers1(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags, const UINT* pCreationNodeMask, IUnknown* const* ppPresentQueue) {
	CleanupRenderTarget();

	return oResizeBuffers1(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags, pCreationNodeMask, ppPresentQueue);
}

static CreateSwapChain oCreateSwapChain;
static HRESULT WINAPI hkCreateSwapChain(IDXGIFactory* pFactory, IUnknown* pDevice, DXGI_SWAP_CHAIN_DESC* pDesc, IDXGISwapChain** ppSwapChain) {
	CleanupRenderTarget();

	return oCreateSwapChain(pFactory, pDevice, pDesc, ppSwapChain);
}

static CreateSwapChainForHwnd oCreateSwapChainForHwnd;
static HRESULT WINAPI hkCreateSwapChainForHwnd(IDXGIFactory* pFactory, IUnknown* pDevice, HWND hWnd, const DXGI_SWAP_CHAIN_DESC1* pDesc, const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* pFullscreenDesc, IDXGIOutput* pRestrictToOutput, IDXGISwapChain1** ppSwapChain) {
	CleanupRenderTarget();

	return oCreateSwapChainForHwnd(pFactory, pDevice, hWnd, pDesc, pFullscreenDesc, pRestrictToOutput, ppSwapChain);
}

static CreateSwapChainForCoreWindow oCreateSwapChainForCoreWindow;
static HRESULT WINAPI hkCreateSwapChainForCoreWindow(IDXGIFactory* pFactory, IUnknown* pDevice, IUnknown* pWindow, const DXGI_SWAP_CHAIN_DESC1* pDesc, IDXGIOutput* pRestrictToOutput, IDXGISwapChain1** ppSwapChain) {
	CleanupRenderTarget();

	return oCreateSwapChainForCoreWindow(pFactory, pDevice, pWindow, pDesc, pRestrictToOutput, ppSwapChain);
}

static CreateSwapChainForComposition oCreateSwapChainForComposition;
static HRESULT WINAPI hkCreateSwapChainForComposition(IDXGIFactory* pFactory, IUnknown* pDevice, const DXGI_SWAP_CHAIN_DESC1* pDesc, IDXGIOutput* pRestrictToOutput, IDXGISwapChain1** ppSwapChain) {
	CleanupRenderTarget();

	return oCreateSwapChainForComposition(pFactory, pDevice, pDesc, pRestrictToOutput, ppSwapChain);
}

bool RendererHooks::Setup()
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

void RendererHooks::Destroy()
{
	MH_DisableHook(MH_ALL_HOOKS);

	std::this_thread::sleep_for(std::chrono::seconds(3)); // Disable hooks and wait a bit for all threads to finish so we dont crash

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