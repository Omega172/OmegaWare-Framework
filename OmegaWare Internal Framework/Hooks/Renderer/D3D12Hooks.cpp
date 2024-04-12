#include "pch.h"
#if FRAMEWORK_RENDER_D3D12 || FRAMEWORK_RENDER_DYNAMIC

// Data
static int const NUM_BACK_BUFFERS = 3;
static ID3D12Device* g_pDevice = NULL;
static IDXGIFactory4* g_pDXGIFactory = NULL;
static IDXGISwapChain3* g_pSwapChain = NULL;
static ID3D12CommandQueue* g_pD3DCommandQueue = NULL;
static ID3D12Resource* g_aResources[NUM_BACK_BUFFERS] = {};
static ID3D12GraphicsCommandList* g_pD3DCommandList = NULL;
static ID3D12DescriptorHeap* g_pD3DRtvDescriptorHeap = NULL;
static ID3D12DescriptorHeap* g_pD3DSrvDescriptorHeap = NULL;
static ID3D12CommandAllocator* g_aCommandAllocators[NUM_BACK_BUFFERS] = {};
static D3D12_CPU_DESCRIPTOR_HANDLE g_aCpuDescriptorHandles[NUM_BACK_BUFFERS] = {};

static DXGI_FORMAT GetCorrectDXGIFormat(DXGI_FORMAT currentFormat) {
    switch (currentFormat) {
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        return DXGI_FORMAT_R8G8B8A8_UNORM;
    }

    return currentFormat;
}

static bool CreateDeviceD3D12(HWND hWnd) {
    // Setup swap chain
    DXGI_SWAP_CHAIN_DESC1 sd = { };
    sd.BufferCount = NUM_BACK_BUFFERS;
    sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.SampleDesc.Count = 1;
    sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    /* // -d3ddebug launch option
    HMODULE dx12 = GetModuleHandleA("d3d12.dll");

    auto pD3D12GetDebugInterface =
        reinterpret_cast<PFN_D3D12_GET_DEBUG_INTERFACE>(
            GetProcAddress(dx12, "D3D12GetDebugInterface"));
    if (pD3D12GetDebugInterface)
    {
        ID3D12Debug* debugController;
        if (SUCCEEDED(pD3D12GetDebugInterface(
            IID_PPV_ARGS(&debugController))))
        {
            debugController->EnableDebugLayer();
        }
    }
    */

    // Create device
    D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
    if (D3D12CreateDevice(NULL, featureLevel, IID_PPV_ARGS(&g_pDevice)) != S_OK)
        return false;

    D3D12_COMMAND_QUEUE_DESC desc = { };
    if (g_pDevice->CreateCommandQueue(&desc, IID_PPV_ARGS(&g_pD3DCommandQueue)) != S_OK)
        return false;

    if (CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&g_pDXGIFactory)) != S_OK) {
        Utils::LogError(Utils::GetLocation(CurrentLoc), "Unable to create DXGIFactory!");
        return false;
    }

    // FAILURE HERE
    IDXGISwapChain1* swapChain1 = NULL;
    if (g_pDXGIFactory->CreateSwapChainForHwnd(g_pD3DCommandQueue, hWnd, &sd, NULL, NULL, &swapChain1) != S_OK) {
        Utils::LogError(Utils::GetLocation(CurrentLoc), "Unable to create SwapChainForHwnd!");
        return false;
    }

    if (swapChain1->QueryInterface(IID_PPV_ARGS(&g_pSwapChain)) != S_OK) {
        Utils::LogError(Utils::GetLocation(CurrentLoc), "Unable to create SwapChain!");
        return false;
    }
    swapChain1->Release();

    return true;
}

static void CreateRenderTarget(IDXGISwapChain* pSwapChain) {
    for (UINT i = 0; i < NUM_BACK_BUFFERS; ++i) {
        ID3D12Resource* pBackBuffer = NULL;
        pSwapChain->GetBuffer(i, IID_PPV_ARGS(&pBackBuffer));
        if (pBackBuffer) {
            DXGI_SWAP_CHAIN_DESC sd;
            pSwapChain->GetDesc(&sd);

            D3D12_RENDER_TARGET_VIEW_DESC desc = { };
            desc.Format = static_cast<DXGI_FORMAT>(GetCorrectDXGIFormat(sd.BufferDesc.Format));
            desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

            g_pDevice->CreateRenderTargetView(pBackBuffer, &desc, g_aCpuDescriptorHandles[i]);
            g_aResources[i] = pBackBuffer;
        }
    }
}

static void CleanupRenderTarget() {
    for (UINT i = 0; i < NUM_BACK_BUFFERS; ++i)
        if (g_aResources[i]) {
            g_aResources[i]->Release();
            g_aResources[i] = NULL;
        }
}

static void CleanupDeviceD3D12() {
    CleanupRenderTarget();

    if (g_pSwapChain) {
        g_pSwapChain->Release();
        g_pSwapChain = NULL;
    }
    for (UINT i = 0; i < NUM_BACK_BUFFERS; ++i) {
        if (g_aCommandAllocators[i]) {
            g_aCommandAllocators[i]->Release();
            g_aCommandAllocators[i] = NULL;
        }
    }
    if (g_pD3DCommandList) {
        g_pD3DCommandList->Release();
        g_pD3DCommandList = NULL;
    }
    if (g_pD3DRtvDescriptorHeap) {
        g_pD3DRtvDescriptorHeap->Release();
        g_pD3DRtvDescriptorHeap = NULL;
    }
    if (g_pD3DSrvDescriptorHeap) {
        g_pD3DSrvDescriptorHeap->Release();
        g_pD3DSrvDescriptorHeap = NULL;
    }
    if (g_pDevice) {
        g_pDevice->Release();
        g_pDevice = NULL;
    }
    if (g_pDXGIFactory) {
        g_pDXGIFactory->Release();
        g_pDXGIFactory = NULL;
    }
}

static void RenderImGui_DX12(IDXGISwapChain3* pSwapChain) {
    if (!ImGui::GetCurrentContext())
        return;

    if (!ImGui::GetIO().BackendRendererUserData) {
        if (SUCCEEDED(pSwapChain->GetDevice(IID_PPV_ARGS(&g_pDevice)))) {
            {
                D3D12_DESCRIPTOR_HEAP_DESC desc = { };
                desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
                desc.NumDescriptors = NUM_BACK_BUFFERS;
                desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
                desc.NodeMask = 1;
                if (g_pDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&g_pD3DRtvDescriptorHeap)) != S_OK)
                    return;

                SIZE_T rtvDescriptorSize = g_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
                D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = g_pD3DRtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
                for (UINT i = 0; i < NUM_BACK_BUFFERS; ++i) {
                    g_aCpuDescriptorHandles[i] = rtvHandle;
                    rtvHandle.ptr += rtvDescriptorSize;
                }
            }

            D3D12_DESCRIPTOR_HEAP_DESC desc = { };
            desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
            desc.NumDescriptors = 1;
            desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
            if (g_pDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&g_pD3DSrvDescriptorHeap)) != S_OK)
                return;

            for (UINT i = 0; i < NUM_BACK_BUFFERS; ++i)
                if (g_pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&g_aCommandAllocators[i])) != S_OK)
                    return;

            if (g_pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, g_aCommandAllocators[0], NULL, IID_PPV_ARGS(&g_pD3DCommandList)) != S_OK ||
                g_pD3DCommandList->Close() != S_OK)
                return;

            ImGui_ImplDX12_Init(g_pDevice, NUM_BACK_BUFFERS, DXGI_FORMAT_R8G8B8A8_UNORM, g_pD3DSrvDescriptorHeap,
                g_pD3DSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), g_pD3DSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
        }
    }

    if (!g_aResources[0])
        CreateRenderTarget(pSwapChain);

    if (!g_pD3DCommandQueue || !g_aResources[0])
        return;

    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    ImGui::PushFont(CurrentFont);
    GUI::Render();
    ImGui::PopFont();

    ImGui::Render();

    UINT backBufferIdx = pSwapChain->GetCurrentBackBufferIndex();
    ID3D12CommandAllocator* commandAllocator = g_aCommandAllocators[backBufferIdx];
    commandAllocator->Reset();

    D3D12_RESOURCE_BARRIER barrier = {
        .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
        .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
        .Transition = {
            .pResource = g_aResources[backBufferIdx],
            .Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
            .StateBefore = D3D12_RESOURCE_STATE_PRESENT,
            .StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET
        },
    };

    g_pD3DCommandList->Reset(commandAllocator, NULL);
    g_pD3DCommandList->ResourceBarrier(1, &barrier);
    g_pD3DCommandList->OMSetRenderTargets(1, &g_aCpuDescriptorHandles[backBufferIdx], FALSE, NULL);
    g_pD3DCommandList->SetDescriptorHeaps(1, &g_pD3DSrvDescriptorHeap);

    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), g_pD3DCommandList);

    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;

    g_pD3DCommandList->ResourceBarrier(1, &barrier);
    g_pD3DCommandList->Close();
    g_pD3DCommandQueue->ExecuteCommandLists(1, reinterpret_cast<ID3D12CommandList* const*>(&g_pD3DCommandList));
}

static Memory::Hook<HRESULT(WINAPI*)(IDXGISwapChain3*, UINT, UINT)> oPresent;
static HRESULT WINAPI hkPresent(IDXGISwapChain3* pSwapChain, UINT SyncInterval, UINT Flags) {

    static std::once_flag once;
    std::call_once(once, [pSwapChain]() {
        g_pSwapChain = pSwapChain;
        DXGI_SWAP_CHAIN_DESC sdesc;
        g_pSwapChain->GetDesc(&sdesc);
        sdesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
        sdesc.OutputWindow = Cheat::wndproc->hwndWindow;
        sdesc.Windowed = ((GetWindowLongPtr(Cheat::wndproc->hwndWindow, GWL_STYLE) & WS_POPUP) != 0) ? false : true;
    });

    RenderImGui_DX12(pSwapChain);

    return oPresent(pSwapChain, SyncInterval, Flags);
}

static Memory::Hook<HRESULT(WINAPI*)(IDXGISwapChain3*, UINT, UINT, const DXGI_PRESENT_PARAMETERS*)> oPresent1;
static HRESULT WINAPI hkPresent1(IDXGISwapChain3* pSwapChain, UINT SyncInterval, UINT PresentFlags, const DXGI_PRESENT_PARAMETERS* pPresentParameters) {
    RenderImGui_DX12(pSwapChain);

    return oPresent1(pSwapChain, SyncInterval, PresentFlags, pPresentParameters);
}

static Memory::Hook<HRESULT(WINAPI*)(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT)> oResizeBuffers;
static HRESULT WINAPI hkResizeBuffers(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags) {
    CleanupRenderTarget();

    return oResizeBuffers(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);
}

static Memory::Hook<HRESULT(WINAPI*)(IDXGISwapChain3*, UINT, UINT, UINT, DXGI_FORMAT, UINT, const UINT*, IUnknown* const*)> oResizeBuffers1;
static HRESULT WINAPI hkResizeBuffers1(IDXGISwapChain3* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags, const UINT* pCreationNodeMask, IUnknown* const* ppPresentQueue) {
    CleanupRenderTarget();

    return oResizeBuffers1(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags, pCreationNodeMask, ppPresentQueue);
}

static Memory::Hook<void(WINAPI*)(ID3D12CommandQueue*, UINT, ID3D12CommandList*)> oExecuteCommandLists;
static void WINAPI hkExecuteCommandLists(ID3D12CommandQueue* pCommandQueue, UINT NumCommandLists, ID3D12CommandList* ppCommandLists) {
    if (!g_pD3DCommandQueue)
        g_pD3DCommandQueue = pCommandQueue;

    return oExecuteCommandLists(pCommandQueue, NumCommandLists, ppCommandLists);
}

static Memory::Hook<HRESULT(WINAPI*)(IDXGIFactory*, IUnknown*, DXGI_SWAP_CHAIN_DESC*, IDXGISwapChain**)> oCreateSwapChain;
static HRESULT WINAPI hkCreateSwapChain(IDXGIFactory* pFactory, IUnknown* pDevice, DXGI_SWAP_CHAIN_DESC* pDesc, IDXGISwapChain** ppSwapChain) {
    CleanupRenderTarget();

    return oCreateSwapChain(pFactory, pDevice, pDesc, ppSwapChain);
}

static Memory::Hook<HRESULT(WINAPI*)(IDXGIFactory*, IUnknown*, HWND, const DXGI_SWAP_CHAIN_DESC1*, const DXGI_SWAP_CHAIN_FULLSCREEN_DESC*, IDXGIOutput*, IDXGISwapChain1**)> oCreateSwapChainForHwnd;
static HRESULT WINAPI hkCreateSwapChainForHwnd(IDXGIFactory* pFactory, IUnknown* pDevice, HWND hWnd, const DXGI_SWAP_CHAIN_DESC1* pDesc, const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* pFullscreenDesc, IDXGIOutput* pRestrictToOutput, IDXGISwapChain1** ppSwapChain) {
    CleanupRenderTarget();

    return oCreateSwapChainForHwnd(pFactory, pDevice, hWnd, pDesc, pFullscreenDesc, pRestrictToOutput, ppSwapChain);
}

static Memory::Hook<HRESULT(WINAPI*)(IDXGIFactory*, IUnknown*, IUnknown*, const DXGI_SWAP_CHAIN_DESC1*, IDXGIOutput*, IDXGISwapChain1**)> oCreateSwapChainForCoreWindow;
static HRESULT WINAPI hkCreateSwapChainForCoreWindow(IDXGIFactory* pFactory, IUnknown* pDevice, IUnknown* pWindow, const DXGI_SWAP_CHAIN_DESC1* pDesc, IDXGIOutput* pRestrictToOutput, IDXGISwapChain1** ppSwapChain) {
    CleanupRenderTarget();

    return oCreateSwapChainForCoreWindow(pFactory, pDevice, pWindow, pDesc, pRestrictToOutput, ppSwapChain);
}

static Memory::Hook<HRESULT(WINAPI*)(IDXGIFactory*, IUnknown*, const DXGI_SWAP_CHAIN_DESC1*, IDXGIOutput*, IDXGISwapChain1**)> oCreateSwapChainForComposition;
static HRESULT WINAPI hkCreateSwapChainForComposition(IDXGIFactory* pFactory, IUnknown* pDevice, const DXGI_SWAP_CHAIN_DESC1* pDesc, IDXGIOutput* pRestrictToOutput, IDXGISwapChain1** ppSwapChain) {
    CleanupRenderTarget();

    return oCreateSwapChainForComposition(pFactory, pDevice, pDesc, pRestrictToOutput, ppSwapChain);
}

bool RendererHooks::D3D12Setup()
{
    WNDCLASSEX WndClass;
    WndClass.cbSize = sizeof(WNDCLASSEX);
    WndClass.style = CS_HREDRAW | CS_VREDRAW;
    WndClass.lpfnWndProc = DefWindowProc;
    WndClass.cbClsExtra = 0;
    WndClass.cbWndExtra = 0;
    WndClass.hInstance = GetModuleHandle(NULL);
    WndClass.hIcon = NULL;
    WndClass.hCursor = NULL;
    WndClass.hbrBackground = NULL;
    WndClass.lpszMenuName = NULL;
    WndClass.lpszClassName = "OmegaWare";
    WndClass.hIconSm = NULL;

    RegisterClassExA(&WndClass);

    HWND hWindow = CreateWindowA(WndClass.lpszClassName, "OmegaWare DirectX Window", WS_OVERLAPPEDWINDOW, 0, 0, 100, 100, NULL, NULL, WndClass.hInstance, NULL);

    if (!CreateDeviceD3D12(hWindow)) {
        Utils::LogError(Utils::GetLocation(CurrentLoc), "Unable to create dx12 device!");
      
        DestroyWindow(hWindow);
        UnregisterClassA(WndClass.lpszClassName, WndClass.hInstance);

        return false;
    }
    DestroyWindow(hWindow);

    // IDXGIFactory
    if (oCreateSwapChain.HookVirtualMethod(hkCreateSwapChain, g_pDXGIFactory, 10) != MH_OK) {
        Utils::LogError(Utils::GetLocation(CurrentLoc), "CreateSwapChain hook couldnt be created!");
        return false;
    }

    if (oCreateSwapChainForHwnd.HookVirtualMethod(hkCreateSwapChainForHwnd, g_pDXGIFactory, 15) != MH_OK) {
        Utils::LogError(Utils::GetLocation(CurrentLoc), "CreateSwapChainForHwnd hook couldnt be created!");
        return false;
    }

    if (oCreateSwapChainForCoreWindow.HookVirtualMethod(hkCreateSwapChainForCoreWindow, g_pDXGIFactory, 16) != MH_OK) {
        Utils::LogError(Utils::GetLocation(CurrentLoc), "CreateSwapChainForCoreWindow hook couldnt be created!");
        return false;
    }

    if (oCreateSwapChainForComposition.HookVirtualMethod(hkCreateSwapChainForComposition, g_pDXGIFactory, 24) != MH_OK) {
        Utils::LogError(Utils::GetLocation(CurrentLoc), "CreateSwapChainForComposition hook couldnt be created!");
        return false;
    }
    // IDXGIFactory

    // SwapChain
    if (oPresent.HookVirtualMethod(hkPresent, g_pSwapChain, 8) != MH_OK) {
        Utils::LogError(Utils::GetLocation(CurrentLoc), "Present hook couldnt be created!");
        return false;
    }

    if (oResizeBuffers.HookVirtualMethod(hkResizeBuffers, g_pSwapChain, 13) != MH_OK) {
        Utils::LogError(Utils::GetLocation(CurrentLoc), "ResizeBuffers hook couldnt be created!");
        return false;
    }

    if (oPresent1.HookVirtualMethod(hkPresent1, g_pSwapChain, 22) != MH_OK) {
        Utils::LogError(Utils::GetLocation(CurrentLoc), "Present1 hook couldnt be created!");
        return false;
    }

    if (oResizeBuffers1.HookVirtualMethod(hkResizeBuffers1, g_pSwapChain, 39) != MH_OK) {
        Utils::LogError(Utils::GetLocation(CurrentLoc), "ResizeBuffers1 hook couldnt be created!");
        return false;
    }
    // SwapChain

    // D3DCommandQueue
    if (oExecuteCommandLists.HookVirtualMethod(hkExecuteCommandLists, g_pD3DCommandQueue, 10) != MH_OK) {
        Utils::LogError(Utils::GetLocation(CurrentLoc), "ResizeBuffers1 hook couldnt be created!");
        return false;
    }
    // D3DCommandQueue


    if (g_pD3DCommandQueue) {
        g_pD3DCommandQueue->Release();
        g_pD3DCommandQueue = NULL;
    }
    CleanupDeviceD3D12();

    return true;
}

void RendererHooks::D3D12Destroy()
{
    oPresent.RemoveHook();
    oPresent1.RemoveHook();
    oResizeBuffers1.RemoveHook();
    oCreateSwapChain.RemoveHook();
    oCreateSwapChainForHwnd.RemoveHook();
    oCreateSwapChainForCoreWindow.RemoveHook();
    oCreateSwapChainForComposition.RemoveHook();
    oExecuteCommandLists.RemoveHook();

    if (ImGui::GetCurrentContext()) {
        if (ImGui::GetIO().BackendRendererUserData)
            ImGui_ImplDX12_Shutdown();

        if (ImGui::GetIO().BackendPlatformUserData)
            ImGui_ImplWin32_Shutdown();

        ImGui::DestroyContext();
    }

    CleanupDeviceD3D12();
}
#endif