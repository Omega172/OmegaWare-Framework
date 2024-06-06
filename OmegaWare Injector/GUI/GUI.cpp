#include <shobjidl.h>
#include <TlHelp32.h>

#include "../Utils/Logging/Logging.h"

#include "GUI.h"
#include "../Libs/ImGUI/imgui.h"
#include "../Libs/ImGUI/imgui_impl_dx9.h"
#include "../Libs/ImGUI/imgui_impl_win32.h"
#include "Styles.h"

#include "../Inject/Inject.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND window, UINT message, WPARAM wideParameter, LPARAM londParameter);

WNDCLASSW stWindowClass = {};
HWND hWnd = nullptr;

PDIRECT3D9 pD3D9 = nullptr;
LPDIRECT3DDEVICE9 pDevice = nullptr;
D3DPRESENT_PARAMETERS presentParams = {};

float flWidth = 500.f;
float flHeight = 305.f;
POINTS pointsPosition = {};

struct Proc
{
	std::string sProcessName;
	DWORD dwProcessID;
	HANDLE hProcess = nullptr;
	bool bSelected = false;
};

std::vector<Proc> Processes;
Proc stSelectedProcess = { "None", 0, nullptr, false };

std::vector<Proc> GetProcesses()
{
	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

	PROCESSENTRY32 stEntry;
	stEntry.dwSize = sizeof(PROCESSENTRY32);

	std::vector<Proc> ProcList;
	if (Process32First(hSnap, &stEntry) == TRUE)
	{
		while (Process32Next(hSnap, &stEntry) == TRUE)
		{
			ProcList.push_back({ stEntry.szExeFile, stEntry.th32ProcessID, OpenProcess(PROCESS_ALL_ACCESS, FALSE, stEntry.th32ProcessID), false });
		}
	}

	CloseHandle(hSnap);
	return ProcList;
}

LRESULT __stdcall GUI::WndProc(HWND hWindow, UINT uiMessage, WPARAM wideParam, LPARAM longParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWindow, uiMessage, wideParam, longParam))
		return true;

	switch (uiMessage)
	{
	case WM_SIZE: {
		if (pDevice && wideParam != SIZE_MINIMIZED)
		{
			presentParams.BackBufferWidth = LOWORD(longParam);
			presentParams.BackBufferHeight = HIWORD(longParam);
			ResetDevice();
		}
	}return false;

	case WM_SYSCOMMAND: {
		if ((wideParam & 0xfff0) == SC_KEYMENU)
			return false;
	}break;

	case WM_DESTROY: {
		PostQuitMessage(0);
	}return false;

	case WM_LBUTTONDOWN: {
		pointsPosition = MAKEPOINTS(longParam);
	}return false;

	case WM_MOUSEMOVE: {
		if (wideParam == MK_LBUTTON)
		{
			const auto points = MAKEPOINTS(longParam);
			auto rect = ::RECT{};

			GetWindowRect(hWnd, &rect);

			rect.left += points.x - pointsPosition.x;
			rect.top += points.y - pointsPosition.y;

			if (pointsPosition.x >= 0 && pointsPosition.x <= flWidth && pointsPosition.y >= 0 && pointsPosition.y <= 19)
				SetWindowPos(hWnd, HWND_TOPMOST, rect.left, rect.top, 0, 0, SWP_SHOWWINDOW || SWP_NOSIZE | SWP_NOZORDER);
		}
	}return false;

	}

	return DefWindowProcW(hWindow, uiMessage, wideParam, longParam);
}

void GUI::CreateWnd(const wchar_t* pwszWindowTitle, const wchar_t* pwszClassName)
{
	stWindowClass.style = CS_CLASSDC;
	stWindowClass.lpfnWndProc = WndProc;
	stWindowClass.cbClsExtra = NULL;
	stWindowClass.cbWndExtra = NULL;
	stWindowClass.hInstance = GetModuleHandleA(NULL);
	stWindowClass.hIcon = NULL;
	stWindowClass.hCursor = NULL;
	stWindowClass.hbrBackground = NULL;
	stWindowClass.lpszMenuName = NULL;
	stWindowClass.lpszClassName = pwszClassName;

	RegisterClassW(&stWindowClass);

	float flHighDPIScaleFactor = ImGui_ImplWin32_GetDpiScaleForHwnd(GetDesktopWindow());
	hWnd = CreateWindowExW(0L, pwszClassName, pwszWindowTitle, WS_POPUP, 100, 100, static_cast<int>(flWidth * flHighDPIScaleFactor), static_cast<int>(flHeight * flHighDPIScaleFactor), NULL, NULL, stWindowClass.hInstance, NULL);

	ShowWindow(hWnd, SW_SHOWDEFAULT);
	UpdateWindow(hWnd);
}

void GUI::DestroyWindow()
{
	DestroyWindow(hWnd);
	UnregisterClassW(stWindowClass.lpszClassName, stWindowClass.hInstance);
}

bool GUI::CreateDevice()
{
	pD3D9 = Direct3DCreate9(D3D_SDK_VERSION);

	if (!pD3D9)
		return false;

	ZeroMemory(&presentParams, sizeof(presentParams));

	presentParams.Windowed = true;
	presentParams.SwapEffect = D3DSWAPEFFECT_DISCARD;
	presentParams.BackBufferFormat = D3DFMT_UNKNOWN;
	presentParams.EnableAutoDepthStencil = TRUE;
	presentParams.AutoDepthStencilFormat = D3DFMT_D16;
	presentParams.PresentationInterval = D3DPRESENT_INTERVAL_ONE;

	if (pD3D9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &presentParams, &pDevice) < 0)
		return false;

	return true;
}

void GUI::ResetDevice()
{
	ImGui_ImplDX9_InvalidateDeviceObjects();

	const auto result = pDevice->Reset(&presentParams);

	if (result == D3DERR_INVALIDCALL)
		IM_ASSERT(0);

	ImGui_ImplDX9_CreateDeviceObjects();
}

void GUI::DestroyDevice()
{
	if (pDevice)
	{
		pDevice->Release();
		pDevice = nullptr;
	}

	if (pD3D9)
	{
		pD3D9->Release();
		pD3D9 = nullptr;
	}
}

void GUI::CreateImGui()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ::ImGui::GetIO();

	io.IniFilename = NULL;

	ImGui::StyleColorsDark();
	SetupStyle();
	ImportFonts();

	ImGui_ImplWin32_Init(hWnd);
	ImGui_ImplDX9_Init(pDevice);
	ImGui_ImplWin32_EnableDpiAwareness();
}

void GUI::DestroyImGui()
{
	ImGui_ImplDX9_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

void GUI::BeginRender()
{
	MSG message;
	while (PeekMessage(&message, 0, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&message);
		DispatchMessageA(&message);
	}

	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	ImGui::PushFont(TahomaFont);
}

void GUI::Render()
{
	ImGui::SetNextWindowPos({ 0, 0 });
	ImGui::SetNextWindowSize(ImVec2(flWidth, flHeight));
	ImGui::Begin("OmegaWare.xyz DLL Injector", &bKeepRunning, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove);
	{
		ImGui::BeginChild("MainChild", { ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y }, ImGuiChildFlags_Border, ImGuiWindowFlags_None);
		{
			static char szDLLPath[MAX_PATH + 1] = "";
			ImGui::InputText("DLL Path", szDLLPath, IM_ARRAYSIZE(szDLLPath), ImGuiInputTextFlags_None, NULL, nullptr);
			ImGui::SameLine();
			if (ImGui::Button("Browse"))
			{
				LogDebugHere("Opening file dialog...");

				HRESULT hRes = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
				if (SUCCEEDED(hRes))
				{
					IFileOpenDialog* pFileOpen;
					hRes = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));

					COMDLG_FILTERSPEC rgSpec[] =
					{
						{L"Dynamic Link Librairy" , L"*.dll"}
					};
					pFileOpen->SetFileTypes(1, rgSpec);

					if (SUCCEEDED(hRes))
					{
						hRes = pFileOpen->Show(NULL);

						if (SUCCEEDED(hRes))
						{
							IShellItem* pItem;
							hRes = pFileOpen->GetResult(&pItem);
							if (SUCCEEDED(hRes))
							{
								PWSTR pszFilePath;
								hRes = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

								// Display the file name to the user.
								if (SUCCEEDED(hRes))
								{
									std::wstring wsContainer = pszFilePath;

									std::string sContainer;
									size_t ullSize;

									sContainer.resize(wsContainer.length());

									wcstombs_s(&ullSize, &sContainer[0], sContainer.size() + 1, wsContainer.c_str(), wsContainer.size());
									strcpy_s(szDLLPath, sContainer.c_str());

									CoTaskMemFree(pszFilePath);
								}
								pItem->Release();
							}
							pFileOpen->Release();
						}
					}
					CoUninitialize();
				}
			}

			static char szSearchTerm[256] = "";

			ImGui::BeginListBox("Processes");
			for (Proc Process : Processes)
			{
				std::string& sProcessName = Process.sProcessName;

				std::string sSearchTerm = szSearchTerm;
				if (!sSearchTerm.empty())
				{
					if (Utils::ToLower(sProcessName).find(Utils::ToLower(szSearchTerm)) == std::string::npos) {
						continue;
					}
				}

				if (ImGui::Selectable(sProcessName.c_str(), Process.bSelected))
				{
					stSelectedProcess = Process;
				}
			}
			ImGui::EndListBox();

			ImGui::InputText("Search", szSearchTerm, 256ull);
			if (ImGui::Button("Refresh Processes")) { Processes = GetProcesses(); }
			
			ImGui::SameLine();
			if (ImGui::Button("Inject"))
			{
				std::string sDLLPath = szDLLPath;
				if (sDLLPath.empty())
				{
					MessageBoxA(NULL, "Injection failed because no DLL was selected.", "No DLL selected", MB_ICONERROR);
				}
				else if (stSelectedProcess.dwProcessID == NULL)
				{
					MessageBoxA(NULL, "Injection failed because no process was selected.", "No process selected", MB_ICONERROR);
				}
				else
				{
					bool bResult = Inject::ManualMap(stSelectedProcess.hProcess, szDLLPath);
				}
			}

			std::string sSelectedProcess = "Selected Process: ";
			sSelectedProcess += stSelectedProcess.sProcessName;
			sSelectedProcess += " - ";
			sSelectedProcess += std::to_string(stSelectedProcess.dwProcessID);
			ImGui::Text(sSelectedProcess.c_str());
		}
		ImGui::EndChild();
	}
	ImGui::End();
}

void GUI::EndRender()
{
	ImGui::PopFont();

	ImGui::EndFrame();

	pDevice->SetRenderState(D3DRS_ZENABLE, false);
	pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, false);
	pDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, false);

	pDevice->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_RGBA(0, 0, 0, 255), 1.0f, 0);

	if (pDevice->BeginScene() >= 0)
	{
		ImGui::Render();
		ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
		pDevice->EndScene();
	}

	const auto result = pDevice->Present(0, 0, 0, 0);

	if (result == D3DERR_DEVICELOST && pDevice->TestCooperativeLevel() == D3DERR_DEVICELOST)
		ResetDevice();
}