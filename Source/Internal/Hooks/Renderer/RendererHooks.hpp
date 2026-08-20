#pragma once
#include "pch.h"
#include <atomic>

class RendererHooks
{
public:
	enum ERendererType {
		NONE,
		D3D11,
		D3D12
	};

#if FRAMEWORK_RENDER_D3D11 || FRAMEWORK_RENDER_DYNAMIC
	bool D3D11Setup();
	void D3D11Destroy();
#endif

#if FRAMEWORK_RENDER_D3D12 || FRAMEWORK_RENDER_DYNAMIC
	bool D3D12Setup();
	void D3D12Destroy();
#endif

	std::atomic<ERendererType> DetectedRenderer{ NONE };

	static ERendererType DetectFromSwapChain(IDXGISwapChain* pSwapChain);

#if FRAMEWORK_RENDER_DYNAMIC
	bool SetupDynamicDetection();

	void PauseDynamicDetection();
	void ResumeDynamicDetectionIfUndetected();

	void D3D11ReleaseUnusedResources();
	void D3D12ReleaseUnusedResources();

	void OnRendererDetected(ERendererType eDetected);
#endif

	bool Setup();

	void Destroy();
};