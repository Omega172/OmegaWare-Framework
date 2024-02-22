#pragma once
#include "pch.h"

#if FRAMEWORK_RENDER_D3D11
#include "Hooks/Renderer/D3D11/D3D11Hooks.h"
#endif

#if FRAMEWORK_RENDER_D3D12
#include "Hooks/Renderer/D3D12/D3D12Hooks.h"
#endif

class RendererHooks
{
private:
	bool Initialized = false;

public:

	bool Setup();

	void Destroy();
};