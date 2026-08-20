#include "pch.h"
#include <mutex>


#if FRAMEWORK_RENDER_D3D11 || FRAMEWORK_RENDER_DYNAMIC

static ID3D11Device* g_pDevice = NULL;
static ID3D11DeviceContext* g_pDeviceContext = NULL;
static ID3D11RenderTargetView* g_pRenderTargetView = NULL;
static IDXGISwapChain* g_pSwapChain = NULL;
static bool g_bShuttingDown = false;
static bool g_bInitialized = false;

static std::recursive_mutex g_hookMutex;

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

		if (!Memory::IsValidObjectPtr(g_pDevice)) {
			Utils::LogError("ID3D11Device is invalid!");
			return;
		}

		desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
		if (FAILED(g_pDevice->CreateRenderTargetView(pBackBuffer, &desc, &g_pRenderTargetView))) {
			desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

			if (FAILED(g_pDevice->CreateRenderTargetView(pBackBuffer, &desc, &g_pRenderTargetView))) {

				if (FAILED(g_pDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_pRenderTargetView))) {
					Utils::LogError("ID3D11Device::CreateRenderTargetView failure!");
				}
			}
		}

		pBackBuffer->Release();
	}
}

static void CleanupRenderTarget() {
	Utils::LogDebug("Called.");

	if (g_pRenderTargetView) {
		g_pRenderTargetView->Release();
		g_pRenderTargetView = NULL;
	}
}

static void CleanupDevice() {
	Utils::LogDebug("Called.");

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

	if (g_bShuttingDown)
		return;

#if FRAMEWORK_RENDER_DYNAMIC
	if (Framework::renderer->DetectedRenderer == RendererHooks::NONE)
	{
		RendererHooks::ERendererType eExpected = RendererHooks::NONE;
		RendererHooks::ERendererType eDetected = RendererHooks::DetectFromSwapChain(pSwapChain);

		if (Framework::renderer->DetectedRenderer.compare_exchange_strong(eExpected, eDetected))
		{
			Utils::LogDebug(eDetected == RendererHooks::D3D11 ? "Detected D3D11 swap chain (via first real Present)." : "Detected D3D12 swap chain (via first real Present).");
			Framework::renderer->OnRendererDetected(eDetected);
		}
	}

	if (Framework::renderer->DetectedRenderer != RendererHooks::D3D11)
		return;
#endif
	if (g_bInitialized && GUI::bPendingFontRebuild) {
		ImGui::GetIO().Fonts->Clear();
		ImportFonts(GUI::flPendingUIScale);

		ImGui_ImplDX11_InvalidateDeviceObjects();
		ImGui_ImplDX11_CreateDeviceObjects();

		CurrentFont = TahomaFont;
		GUI::bPendingFontRebuild = false;
	}

	if (!ImGui::GetIO().BackendRendererUserData) {

		if (!SUCCEEDED(pSwapChain->GetDevice(IID_PPV_ARGS(&g_pDevice)))) {
			Utils::LogDebug("IDXGIDeviceSubObject::GetDevice failure!");
			return;
		}

		g_pDevice->GetImmediateContext(&g_pDeviceContext);
		ImGui_ImplDX11_Init(g_pDevice, g_pDeviceContext);
		g_bInitialized = true;
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

static Hooking::Hook<HRESULT(WINAPI*)(IDXGISwapChain*, UINT, UINT)> oPresent;
static HRESULT WINAPI hkPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags) {
	std::lock_guard<std::recursive_mutex> lock(g_hookMutex);

	RenderImGui(pSwapChain);

	return oPresent(pSwapChain, SyncInterval, Flags);
}

static Hooking::Hook<HRESULT(WINAPI*)(IDXGISwapChain*, UINT, UINT, const DXGI_PRESENT_PARAMETERS*)> oPresent1;
static HRESULT WINAPI hkPresent1(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT PresentFlags, const DXGI_PRESENT_PARAMETERS* pPresentParameters) {
	std::lock_guard<std::recursive_mutex> lock(g_hookMutex);

	RenderImGui(pSwapChain);

	return oPresent1(pSwapChain, SyncInterval, PresentFlags, pPresentParameters);
}

static Hooking::Hook<HRESULT(WINAPI*)(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT)> oResizeBuffers;
static HRESULT WINAPI hkResizeBuffers(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags) {
	std::lock_guard<std::recursive_mutex> lock(g_hookMutex);

	CleanupRenderTarget();

	return oResizeBuffers(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);
}

static Hooking::Hook<HRESULT(WINAPI*)(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT, const UINT*, IUnknown* const*)> oResizeBuffers1;
static HRESULT WINAPI hkResizeBuffers1(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags, const UINT* pCreationNodeMask, IUnknown* const* ppPresentQueue) {
	std::lock_guard<std::recursive_mutex> lock(g_hookMutex);

	CleanupRenderTarget();

	return oResizeBuffers1(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags, pCreationNodeMask, ppPresentQueue);
}

static Hooking::Hook<HRESULT(WINAPI*)(IDXGIFactory*, IUnknown*, DXGI_SWAP_CHAIN_DESC*, IDXGISwapChain**)> oCreateSwapChain;
static HRESULT WINAPI hkCreateSwapChain(IDXGIFactory* pFactory, IUnknown* pDevice, DXGI_SWAP_CHAIN_DESC* pDesc, IDXGISwapChain** ppSwapChain) {
	std::lock_guard<std::recursive_mutex> lock(g_hookMutex);

	CleanupRenderTarget();

	return oCreateSwapChain(pFactory, pDevice, pDesc, ppSwapChain);
}

static Hooking::Hook<HRESULT(WINAPI*)(IDXGIFactory*, IUnknown*, HWND, const DXGI_SWAP_CHAIN_DESC1*, const DXGI_SWAP_CHAIN_FULLSCREEN_DESC*, IDXGIOutput*, IDXGISwapChain1**)> oCreateSwapChainForHwnd;
static HRESULT WINAPI hkCreateSwapChainForHwnd(IDXGIFactory* pFactory, IUnknown* pDevice, HWND hWnd, const DXGI_SWAP_CHAIN_DESC1* pDesc, const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* pFullscreenDesc, IDXGIOutput* pRestrictToOutput, IDXGISwapChain1** ppSwapChain) {
	std::lock_guard<std::recursive_mutex> lock(g_hookMutex);

	CleanupRenderTarget();

	return oCreateSwapChainForHwnd(pFactory, pDevice, hWnd, pDesc, pFullscreenDesc, pRestrictToOutput, ppSwapChain);
}

static Hooking::Hook<HRESULT(WINAPI*)(IDXGIFactory*, IUnknown*, IUnknown*, const DXGI_SWAP_CHAIN_DESC1*, IDXGIOutput*, IDXGISwapChain1**)> oCreateSwapChainForCoreWindow;
static HRESULT WINAPI hkCreateSwapChainForCoreWindow(IDXGIFactory* pFactory, IUnknown* pDevice, IUnknown* pWindow, const DXGI_SWAP_CHAIN_DESC1* pDesc, IDXGIOutput* pRestrictToOutput, IDXGISwapChain1** ppSwapChain) {
	std::lock_guard<std::recursive_mutex> lock(g_hookMutex);

	CleanupRenderTarget();

	return oCreateSwapChainForCoreWindow(pFactory, pDevice, pWindow, pDesc, pRestrictToOutput, ppSwapChain);
}

static Hooking::Hook<HRESULT(WINAPI*)(IDXGIFactory*, IUnknown*, const DXGI_SWAP_CHAIN_DESC1*, IDXGIOutput*, IDXGISwapChain1**)> oCreateSwapChainForComposition;
static HRESULT WINAPI hkCreateSwapChainForComposition(IDXGIFactory* pFactory, IUnknown* pDevice, const DXGI_SWAP_CHAIN_DESC1* pDesc, IDXGIOutput* pRestrictToOutput, IDXGISwapChain1** ppSwapChain) {
	std::lock_guard<std::recursive_mutex> lock(g_hookMutex);

	CleanupRenderTarget();

	return oCreateSwapChainForComposition(pFactory, pDevice, pDesc, pRestrictToOutput, ppSwapChain);
}

struct D3D11HookTargets
{
	void* pCreateSwapChain = nullptr;
	void* pCreateSwapChainForHwnd = nullptr;
	void* pCreateSwapChainForCoreWindow = nullptr;
	void* pCreateSwapChainForComposition = nullptr;
	void* pPresent = nullptr;
	void* pResizeBuffers = nullptr;
	void* pPresent1 = nullptr;
	void* pResizeBuffers1 = nullptr;
};

static bool ResolveD3D11HookTargets(D3D11HookTargets& out)
{
	IDXGIDevice* pDXGIDevice = NULL;
	g_pDevice->QueryInterface(IID_PPV_ARGS(&pDXGIDevice));

	IDXGIAdapter* pDXGIAdapter = NULL;
	if (pDXGIDevice)
		pDXGIDevice->GetAdapter(&pDXGIAdapter);

	IDXGIFactory* pIDXGIFactory = NULL;
	if (pDXGIAdapter)
		pDXGIAdapter->GetParent(IID_PPV_ARGS(&pIDXGIFactory));

	bool bResolved = false;
	if (pIDXGIFactory)
	{
		out.pCreateSwapChain = Memory::GetVirtualMethod(pIDXGIFactory, 10);
		out.pCreateSwapChainForHwnd = Memory::GetVirtualMethod(pIDXGIFactory, 15);
		out.pCreateSwapChainForCoreWindow = Memory::GetVirtualMethod(pIDXGIFactory, 16);
		out.pCreateSwapChainForComposition = Memory::GetVirtualMethod(pIDXGIFactory, 24);

		out.pPresent = Memory::GetVirtualMethod(g_pSwapChain, 8);
		out.pResizeBuffers = Memory::GetVirtualMethod(g_pSwapChain, 13);
		out.pPresent1 = Memory::GetVirtualMethod(g_pSwapChain, 22);
		out.pResizeBuffers1 = Memory::GetVirtualMethod(g_pSwapChain, 39);

		bResolved = out.pCreateSwapChain && out.pCreateSwapChainForHwnd && out.pCreateSwapChainForCoreWindow &&
			out.pCreateSwapChainForComposition && out.pPresent && out.pResizeBuffers && out.pPresent1 && out.pResizeBuffers1;
	}

	if (pIDXGIFactory)
		pIDXGIFactory->Release();
	if (pDXGIAdapter)
		pDXGIAdapter->Release();
	if (pDXGIDevice)
		pDXGIDevice->Release();

	return bResolved;
}

bool RendererHooks::D3D11Setup()
{
	const bool bDeviceOk = CreateDevice(Framework::wndproc.get()->hwndWindow);

	if (!bDeviceOk) {
		Utils::LogError("CreateDevice failure!");
		return false;
	}

	D3D11HookTargets targets{};
	if (!ResolveD3D11HookTargets(targets)) {
		Utils::LogError("Failed to resolve D3D11/DXGI vtable addresses!");
		return false;
	}

	Hooking::HookBatch batch;

	if (!batch.Install(oCreateSwapChain, hkCreateSwapChain, targets.pCreateSwapChain)) {
		Utils::LogHook("CreateSwapChain", oCreateSwapChain.GetStatus());
		return false;
	}

	if (!batch.Install(oCreateSwapChainForHwnd, hkCreateSwapChainForHwnd, targets.pCreateSwapChainForHwnd)) {
		Utils::LogHook("CreateSwapChainForHwnd", oCreateSwapChainForHwnd.GetStatus());
		return false;
	}

	if (!batch.Install(oCreateSwapChainForCoreWindow, hkCreateSwapChainForCoreWindow, targets.pCreateSwapChainForCoreWindow)) {
		Utils::LogHook("CreateSwapChainForCoreWindow", oCreateSwapChainForCoreWindow.GetStatus());
		return false;
	}

	if (!batch.Install(oCreateSwapChainForComposition, hkCreateSwapChainForComposition, targets.pCreateSwapChainForComposition)) {
		Utils::LogHook("CreateSwapChainForComposition", oCreateSwapChainForComposition.GetStatus());
		return false;
	}

	if (!batch.Install(oPresent, hkPresent, targets.pPresent)) {
		Utils::LogHook("Present", oPresent.GetStatus());
		return false;
	}

	if (!batch.Install(oResizeBuffers, hkResizeBuffers, targets.pResizeBuffers)) {
		Utils::LogHook("ResizeBuffers", oResizeBuffers.GetStatus());
		return false;
	}

	if (!batch.Install(oPresent1, hkPresent1, targets.pPresent1)) {
		Utils::LogHook("Present1", oPresent1.GetStatus());
		return false;
	}

	if (!batch.Install(oResizeBuffers1, hkResizeBuffers1, targets.pResizeBuffers1)) {
		Utils::LogHook("ResizeBuffers1", oResizeBuffers1.GetStatus());
		return false;
	}

	batch.Commit();

	CleanupRenderTarget();

	return true;
}

void RendererHooks::D3D11Destroy()
{
	g_bShuttingDown = true;

	{
		std::lock_guard<std::recursive_mutex> lock(g_hookMutex);

		oPresent.Remove();
		oPresent1.Remove();
		oResizeBuffers.Remove();
		oResizeBuffers1.Remove();
		oCreateSwapChain.Remove();
		oCreateSwapChainForHwnd.Remove();
		oCreateSwapChainForCoreWindow.Remove();
		oCreateSwapChainForComposition.Remove();
	}

	if (g_bInitialized && ImGui::GetCurrentContext()) {
		ImGui_ImplDX11_Shutdown();

		if (ImGui::GetIO().BackendPlatformUserData)
			ImGui_ImplWin32_Shutdown();

		ImGui::DestroyContext();
	}

	CleanupDevice();
	g_bInitialized = false;
}

#if FRAMEWORK_RENDER_DYNAMIC
void RendererHooks::D3D11ReleaseUnusedResources()
{
	CleanupDevice();
}
#endif

#endif
