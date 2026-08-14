#pragma once
#include "pch.h"

class RendererHooks
{
private:
#if FRAMEWORK_RENDER_D3D11 || FRAMEWORK_RENDER_DYNAMIC
	bool D3D11Setup();
	void D3D11Destroy();
	void D3D11RebuildFontTexture();
#endif

#if FRAMEWORK_RENDER_D3D12 || FRAMEWORK_RENDER_DYNAMIC
	bool D3D12Setup();
	void D3D12Destroy();
	void D3D12RebuildFontTexture();
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

	// Re-uploads the GPU-side font atlas texture after the caller has already rebuilt it
	// (io.Fonts->Clear() + re-adding fonts, e.g. via ImportFonts()) - see GUI.cpp's UI_SCALE
	// handling. Dispatches to whichever backend is actually active.
	void RebuildFontTexture();
};