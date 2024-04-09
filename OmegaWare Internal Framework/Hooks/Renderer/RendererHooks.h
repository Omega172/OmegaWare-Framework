#pragma once
#include "pch.h"

class RendererHooks
{
private:
#if FRAMEWORK_RENDER_D3D11 || FRAMEWORK_RENDER_DYNAMIC
	bool D3D11Setup();
	void D3D11Destroy();
#endif

#if FRAMEWORK_RENDER_D3D12 || FRAMEWORK_RENDER_DYNAMIC
	bool D3D12Setup();
	void D3D12Destroy();
#endif

public:
	enum ERendererType {
		NONE,
		D3D11,
		D3D12
	};

	ERendererType iActiveRendererType = NONE;

	bool Setup();

	void Destroy();
};