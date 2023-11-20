#pragma once
#include "pch.h"
#if FRAMEWORK_RENDER_D3D11

#include <D3D11.h>
#include <DXGI.h>

#include "../../ImGUI/Styles.h"

typedef HRESULT(__stdcall* Present) (IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
typedef LRESULT(CALLBACK* WNDPROC)(HWND, UINT, WPARAM, LPARAM);

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

Present oPresent;
WNDPROC oWndProc;

HWND window = NULL;
ID3D11Device* pDevice = NULL;
ID3D11DeviceContext* pContext = NULL;
ID3D11RenderTargetView* mainRenderTargetView;

bool hkInit = false;

void InitImGui()
{
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags = ImGuiConfigFlags_NoMouseCursorChange;
	ImGui_ImplWin32_Init(window);
	ImGui_ImplDX11_Init(pDevice, pContext);

	SetupStyle();
	ImportFonts();
}

LRESULT __stdcall WndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam);

	return (GUI::bMenuOpen ? true : CallWindowProcA(oWndProc, hWnd, uMsg, wParam, lParam));
}

HRESULT __stdcall hkPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
{
	if (!hkInit)
	{
		if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&pDevice)))
		{
			pDevice->GetImmediateContext(&pContext);
			DXGI_SWAP_CHAIN_DESC sd;
			pSwapChain->GetDesc(&sd);
			window = sd.OutputWindow;
			ID3D11Texture2D* pBackBuffer;
			pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
			if (pBackBuffer != NULL)
				pDevice->CreateRenderTargetView(pBackBuffer, NULL, &mainRenderTargetView);
			pBackBuffer->Release();
			oWndProc = (WNDPROC)SetWindowLongPtrA(window, GWLP_WNDPROC, (LONG_PTR)WndProc);
			InitImGui();
			hkInit = true;
			return oPresent(pSwapChain, SyncInterval, Flags);
		}
		else
			return oPresent(pSwapChain, SyncInterval, Flags);
	}

	GUI::BeginRender();
	ImGui::PushFont(tahomaFont);
	GUI::Render();
	ImGui::PopFont();
	GUI::EndRender();

	pContext->OMSetRenderTargets(1, &mainRenderTargetView, NULL);
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	if (!Cheat::bShouldRun)
	{
		pDevice->Release();
		pContext->Release();
		pSwapChain->Release();
		oWndProc = (WNDPROC)SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)(oWndProc));
		oPresent(pSwapChain, SyncInterval, Flags);
		kiero::shutdown();
		return 0;
	}

	return oPresent(pSwapChain, SyncInterval, Flags);
}
#endif