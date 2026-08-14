#include "pch.h"


static std::once_flag g_InputInit;

static WNDPROC oWndProc;
static LRESULT hkWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    std::call_once(g_InputInit, [hWnd]() {
        ImGui::CreateContext();
        ImGui_ImplWin32_Init(hWnd);
        ImGuiIO& io = ImGui::GetIO();
        io.IniFilename = io.LogFilename = nullptr;
        io.ConfigFlags = ImGuiConfigFlags_NoMouseCursorChange;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

        ImGui::StyleColorsDark();

        SetupStyle();
        ImportFonts();
    });

    extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam);

    if (GUI::bMenuOpen) {
        switch (uMsg) {
        case WM_MOUSEMOVE:
        case WM_NCMOUSEMOVE:
        case WM_MOUSELEAVE:
        case WM_NCMOUSELEAVE:
        case WM_LBUTTONDOWN:
        case WM_LBUTTONDBLCLK:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONDBLCLK:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONDBLCLK:
        case WM_XBUTTONDOWN:
        case WM_XBUTTONDBLCLK:
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        case WM_MBUTTONUP:
        case WM_XBUTTONUP:
        case WM_MOUSEWHEEL:
        case WM_MOUSEHWHEEL:
        case WM_KEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_SETFOCUS:
        case WM_KILLFOCUS:
        case WM_CHAR:
        case WM_SETCURSOR:
        case WM_DEVICECHANGE:
            return true;
        }
    }

    return CallWindowProcA(oWndProc, hWnd, uMsg, wParam, lParam);
}

static BOOL CALLBACK EnumWindowsCallback(HWND handle, LPARAM lParam) {
    const auto isMainWindow = [handle]() {
        return GetWindow(handle, GW_OWNER) == nullptr && IsWindowVisible(handle) && handle != GetConsoleWindow();
    };

    DWORD pID = 0;
    GetWindowThreadProcessId(handle, &pID);

    if (GetCurrentProcessId() != pID || !isMainWindow()) {
        return TRUE;
    }

    *reinterpret_cast<HWND*>(lParam) = handle;
    return FALSE;
}

bool WndProcHooks::Setup()
{
    dwProcessId = GetCurrentProcessId();

    // When loaded very early in process startup (e.g. via Proxy, which attaches before the
    // host application has created its main window), EnumWindows can legitimately find nothing
    // on the first try. Retry for a while instead of failing immediately.
    constexpr auto kRetryInterval = std::chrono::milliseconds(250);
    constexpr auto kMaxWait = std::chrono::seconds(30);

    const auto tStart = std::chrono::steady_clock::now();
    do {
        EnumWindows(EnumWindowsCallback, reinterpret_cast<LPARAM>(&hwndWindow));
        if (hwndWindow)
            break;

        std::this_thread::sleep_for(kRetryInterval);
    } while (std::chrono::steady_clock::now() - tStart < kMaxWait);

    if (!hwndWindow) {
        Utils::LogError("EnumWindows failure! No window found for this process after waiting.");
        return false;
    }

    Utils::LogDebug(std::format("Window: {:#010x}", reinterpret_cast<uintptr_t>(hwndWindow)));
    oWndProc = reinterpret_cast<WNDPROC>(SetWindowLongPtr(hwndWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(hkWndProc)));

    return true;
}

void WndProcHooks::Destroy()
{
    if (!oWndProc)
        return;

    SetWindowLongPtr(hwndWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(oWndProc));
}