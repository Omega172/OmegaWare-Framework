#include "pch.h"

RendererHooks::ERendererType RendererHooks::DetectFromSwapChain(IDXGISwapChain* pSwapChain)
{
	ID3D12Device* pD3D12Device = nullptr;
	if (SUCCEEDED(pSwapChain->GetDevice(IID_PPV_ARGS(&pD3D12Device))))
	{
		pD3D12Device->Release();
		return D3D12;
	}

	ID3D11Device* pD3D11Device = nullptr;
	if (SUCCEEDED(pSwapChain->GetDevice(IID_PPV_ARGS(&pD3D11Device))))
	{
		pD3D11Device->Release();
		return D3D11;
	}

	return NONE;
}

#if FRAMEWORK_RENDER_DYNAMIC

namespace
{
	Hooking::Hook<HRESULT(WINAPI*)(IDXGIFactory*, IUnknown*, DXGI_SWAP_CHAIN_DESC*, IDXGISwapChain**)> oDetectCreateSwapChain;
	Hooking::Hook<HRESULT(WINAPI*)(IDXGIFactory*, IUnknown*, HWND, const DXGI_SWAP_CHAIN_DESC1*, const DXGI_SWAP_CHAIN_FULLSCREEN_DESC*, IDXGIOutput*, IDXGISwapChain1**)> oDetectCreateSwapChainForHwnd;
	Hooking::Hook<HRESULT(WINAPI*)(IDXGIFactory*, IUnknown*, IUnknown*, const DXGI_SWAP_CHAIN_DESC1*, IDXGIOutput*, IDXGISwapChain1**)> oDetectCreateSwapChainForCoreWindow;
	Hooking::Hook<HRESULT(WINAPI*)(IDXGIFactory*, IUnknown*, const DXGI_SWAP_CHAIN_DESC1*, IDXGIOutput*, IDXGISwapChain1**)> oDetectCreateSwapChainForComposition;

	void DetectAndRemoveHooks(IDXGISwapChain* pRealSwapChain)
	{
		RendererHooks::ERendererType eExpected = RendererHooks::NONE;
		RendererHooks::ERendererType eDetected = RendererHooks::DetectFromSwapChain(pRealSwapChain);

		if (Framework::renderer->DetectedRenderer.compare_exchange_strong(eExpected, eDetected))
		{
			Utils::LogDebug(eDetected == RendererHooks::D3D12 ? "Detected D3D12 swap chain (via CreateSwapChain*)." : "Detected D3D11 swap chain (via CreateSwapChain*).");
			Framework::renderer->OnRendererDetected(eDetected);
		}

		oDetectCreateSwapChain.Remove();
		oDetectCreateSwapChainForHwnd.Remove();
		oDetectCreateSwapChainForCoreWindow.Remove();
		oDetectCreateSwapChainForComposition.Remove();
	}

	HRESULT WINAPI hkDetectCreateSwapChain(IDXGIFactory* pFactory, IUnknown* pDevice, DXGI_SWAP_CHAIN_DESC* pDesc, IDXGISwapChain** ppSwapChain)
	{
		HRESULT hr = oDetectCreateSwapChain(pFactory, pDevice, pDesc, ppSwapChain);
		if (SUCCEEDED(hr) && ppSwapChain && *ppSwapChain)
			DetectAndRemoveHooks(*ppSwapChain);
		return hr;
	}

	HRESULT WINAPI hkDetectCreateSwapChainForHwnd(IDXGIFactory* pFactory, IUnknown* pDevice, HWND hWnd, const DXGI_SWAP_CHAIN_DESC1* pDesc, const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* pFullscreenDesc, IDXGIOutput* pRestrictToOutput, IDXGISwapChain1** ppSwapChain)
	{
		HRESULT hr = oDetectCreateSwapChainForHwnd(pFactory, pDevice, hWnd, pDesc, pFullscreenDesc, pRestrictToOutput, ppSwapChain);
		if (SUCCEEDED(hr) && ppSwapChain && *ppSwapChain)
			DetectAndRemoveHooks(*ppSwapChain);
		return hr;
	}

	HRESULT WINAPI hkDetectCreateSwapChainForCoreWindow(IDXGIFactory* pFactory, IUnknown* pDevice, IUnknown* pWindow, const DXGI_SWAP_CHAIN_DESC1* pDesc, IDXGIOutput* pRestrictToOutput, IDXGISwapChain1** ppSwapChain)
	{
		HRESULT hr = oDetectCreateSwapChainForCoreWindow(pFactory, pDevice, pWindow, pDesc, pRestrictToOutput, ppSwapChain);
		if (SUCCEEDED(hr) && ppSwapChain && *ppSwapChain)
			DetectAndRemoveHooks(*ppSwapChain);
		return hr;
	}

	HRESULT WINAPI hkDetectCreateSwapChainForComposition(IDXGIFactory* pFactory, IUnknown* pDevice, const DXGI_SWAP_CHAIN_DESC1* pDesc, IDXGIOutput* pRestrictToOutput, IDXGISwapChain1** ppSwapChain)
	{
		HRESULT hr = oDetectCreateSwapChainForComposition(pFactory, pDevice, pDesc, pRestrictToOutput, ppSwapChain);
		if (SUCCEEDED(hr) && ppSwapChain && *ppSwapChain)
			DetectAndRemoveHooks(*ppSwapChain);
		return hr;
	}
}

bool RendererHooks::SetupDynamicDetection()
{
	IDXGIFactory* pFactory = nullptr;
	if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&pFactory))))
	{
		Utils::LogError("CreateDXGIFactory1 failure!");
		return false;
	}

	void* pCreateSwapChain = Memory::GetVirtualMethod(pFactory, 10);
	void* pCreateSwapChainForHwnd = Memory::GetVirtualMethod(pFactory, 15);
	void* pCreateSwapChainForCoreWindow = Memory::GetVirtualMethod(pFactory, 16);
	void* pCreateSwapChainForComposition = Memory::GetVirtualMethod(pFactory, 24);

	pFactory->Release();

	if (!pCreateSwapChain || !pCreateSwapChainForHwnd || !pCreateSwapChainForCoreWindow || !pCreateSwapChainForComposition)
	{
		Utils::LogError("Failed to resolve DXGI factory vtable addresses for renderer detection!");
		return false;
	}

	Hooking::HookBatch batch;

	if (!batch.Install(oDetectCreateSwapChain, hkDetectCreateSwapChain, pCreateSwapChain)) {
		Utils::LogHook("DetectCreateSwapChain", oDetectCreateSwapChain.GetStatus());
		return false;
	}

	if (!batch.Install(oDetectCreateSwapChainForHwnd, hkDetectCreateSwapChainForHwnd, pCreateSwapChainForHwnd)) {
		Utils::LogHook("DetectCreateSwapChainForHwnd", oDetectCreateSwapChainForHwnd.GetStatus());
		return false;
	}

	if (!batch.Install(oDetectCreateSwapChainForCoreWindow, hkDetectCreateSwapChainForCoreWindow, pCreateSwapChainForCoreWindow)) {
		Utils::LogHook("DetectCreateSwapChainForCoreWindow", oDetectCreateSwapChainForCoreWindow.GetStatus());
		return false;
	}

	if (!batch.Install(oDetectCreateSwapChainForComposition, hkDetectCreateSwapChainForComposition, pCreateSwapChainForComposition)) {
		Utils::LogHook("DetectCreateSwapChainForComposition", oDetectCreateSwapChainForComposition.GetStatus());
		return false;
	}

	batch.Commit();

	Utils::LogDebug("Renderer detection hooks installed - waiting for the game's real swap chain.");
	return true;
}

void RendererHooks::PauseDynamicDetection()
{
	oDetectCreateSwapChain.Remove();
	oDetectCreateSwapChainForHwnd.Remove();
	oDetectCreateSwapChainForCoreWindow.Remove();
	oDetectCreateSwapChainForComposition.Remove();
}

void RendererHooks::ResumeDynamicDetectionIfUndetected()
{
	if (DetectedRenderer != NONE)
		return;

	SetupDynamicDetection();
}

void RendererHooks::OnRendererDetected(ERendererType eDetected)
{
	switch (eDetected)
	{
	case D3D11:
		Utils::LogDebug("D3D11 is the active renderer - destroying D3D12's unused resources.");
		D3D12ReleaseUnusedResources();
		break;
	case D3D12:
		Utils::LogDebug("D3D12 is the active renderer - destroying D3D11's unused resources.");
		D3D11ReleaseUnusedResources();
		break;
	default:
		break;
	}
}

#endif

bool RendererHooks::Setup()
{
#if FRAMEWORK_RENDER_DYNAMIC
	const bool bDetectionOk = SetupDynamicDetection();

	PauseDynamicDetection();

	const bool bD3D11Ok = D3D11Setup();
	const bool bD3D12Ok = D3D12Setup();

	ResumeDynamicDetectionIfUndetected();

	if (!bDetectionOk && !bD3D11Ok && !bD3D12Ok) {
		Utils::LogError("Failed to install renderer detection or either renderer's hooks!");
		return false;
	}

	return true;
#elif FRAMEWORK_RENDER_D3D12
	return D3D12Setup();
#elif FRAMEWORK_RENDER_D3D11
	return D3D11Setup();
#endif
}

void RendererHooks::Destroy()
{
#if FRAMEWORK_RENDER_DYNAMIC
	oDetectCreateSwapChain.Remove();
	oDetectCreateSwapChainForHwnd.Remove();
	oDetectCreateSwapChainForCoreWindow.Remove();
	oDetectCreateSwapChainForComposition.Remove();

	D3D12Destroy();
	D3D11Destroy();
#elif FRAMEWORK_RENDER_D3D12
	return D3D12Destroy();
#elif FRAMEWORK_RENDER_D3D11
	return D3D11Destroy();
#endif
}

