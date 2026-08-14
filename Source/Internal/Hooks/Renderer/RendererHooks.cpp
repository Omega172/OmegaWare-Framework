#include "pch.h"

bool RendererHooks::Setup()
{
#if FRAMEWORK_RENDER_DYNAMIC
	// Whether d3d12.dll/d3d11.dll are loaded doesn't reliably tell you which one a game's real
	// swap chain uses - some games load d3d12.dll for an unrelated subsystem (video decode is a
	// common one) while actually rendering through D3D11. Hook both unconditionally instead;
	// each Setup() only touches its own throwaway device/swap chain to resolve vtable
	// addresses, so whichever API the game never actually uses simply never has its Present
	// hook called. iActiveRendererType then just reflects whichever one(s) installed.
	const bool bD3D11Ok = D3D11Setup();
	const bool bD3D12Ok = D3D12Setup();

	if (bD3D11Ok)
		Utils::LogDebug("D3D11 hooks installed.");
	if (bD3D12Ok)
		Utils::LogDebug("D3D12 hooks installed.");

	if (!bD3D11Ok && !bD3D12Ok) {
		Utils::LogError("Failed to install either D3D11 or D3D12 hooks!");
		iActiveRendererType = NONE;
		return false;
	}

	iActiveRendererType = bD3D12Ok ? D3D12 : D3D11;
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
	// Both may have been installed (see Setup()), so both need tearing down; each Destroy() is
	// already a safe no-op if its own Setup() never ran.
	D3D11Destroy();
	D3D12Destroy();
#elif FRAMEWORK_RENDER_D3D12
	return D3D12Destroy();
#elif FRAMEWORK_RENDER_D3D11
	return D3D11Destroy();
#endif
}

void RendererHooks::RebuildFontTexture()
{
#if FRAMEWORK_RENDER_DYNAMIC
	if (iActiveRendererType == D3D11)
		D3D11RebuildFontTexture();
	else if (iActiveRendererType == D3D12)
		D3D12RebuildFontTexture();
#elif FRAMEWORK_RENDER_D3D12
	D3D12RebuildFontTexture();
#elif FRAMEWORK_RENDER_D3D11
	D3D11RebuildFontTexture();
#endif
}
