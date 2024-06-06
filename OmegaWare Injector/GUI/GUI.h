#pragma once
#include <d3d9.h>
#pragma comment (lib, "d3d9.lib")
#include <Windows.h>

namespace GUI
{
	inline bool bKeepRunning = true;

	LRESULT __stdcall WndProc(HWND hWindow, UINT uiMessage, WPARAM wideParam, LPARAM longParam);

	void CreateWnd(const wchar_t* pwszWindowTitle, const wchar_t* pwszClassName);
	void DestroyWindow();

	bool CreateDevice();
	void ResetDevice();
	void DestroyDevice();

	void CreateImGui();
	void DestroyImGui();

	void BeginRender();
	void Render();
	void EndRender();
};