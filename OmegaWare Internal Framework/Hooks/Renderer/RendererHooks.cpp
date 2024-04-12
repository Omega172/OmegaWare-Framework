#include "pch.h"

bool RendererHooks::Setup()
{
#if FRAMEWORK_RENDER_DYNAMIC
	if (Memory::GetModule("d3d12.dll"))
	{
		iActiveRendererType = D3D12;
		Utils::LogError(Utils::GetLocation(CurrentLoc), (std::stringstream() << "D3D12 Found @ " << Memory::GetModule("d3d12.dll")).str());
	}
	else if (Memory::GetModule("d3d11.dll"))
	{
		iActiveRendererType = D3D11;
		Utils::LogDebug(Utils::GetLocation(CurrentLoc), "D3D11 found!");
	}
	else
	{
		Utils::LogError(Utils::GetLocation(CurrentLoc), "Couldnt find renderer LAWL!");
	}

	switch (iActiveRendererType) {
	default:
		return false;

	case D3D12:
		return D3D12Setup();

	case D3D11:
		return D3D11Setup();

	}
#elif FRAMEWORK_RENDER_D3D12
	return D3D12Setup();
#elif FRAMEWORK_RENDER_D3D11
	return D3D11Setup();
#endif
}

void RendererHooks::Destroy()
{
#if FRAMEWORK_RENDER_DYNAMIC
	switch (iActiveRendererType) {
	default:
		return;

	case D3D12:
		return D3D12Destroy();

	case D3D11:
		return D3D11Destroy();

	}
#elif FRAMEWORK_RENDER_D3D12
	return D3D12Destroy();
#elif FRAMEWORK_RENDER_D3D11
	return D3D11Destroy();
#endif
}
