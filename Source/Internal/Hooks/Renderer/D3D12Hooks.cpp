#include "pch.h"
#include <mutex>

#if FRAMEWORK_RENDER_D3D12 || FRAMEWORK_RENDER_DYNAMIC

struct FrameContext
{
	ID3D12CommandAllocator* pCommandAllocator = nullptr;
	ID3D12Resource* pBackBuffer = nullptr;
	D3D12_CPU_DESCRIPTOR_HANDLE rtvDescriptor = {};
};

static ID3D12Device* g_pd3dDevice = nullptr;
static ID3D12DescriptorHeap* g_pd3dRtvDescHeap = nullptr;
static ID3D12DescriptorHeap* g_pd3dSrvDescHeap = nullptr;
static ID3D12CommandQueue* g_pd3dCommandQueue = nullptr;
static ID3D12GraphicsCommandList* g_pd3dCommandList = nullptr;
static IDXGISwapChain3* g_pSwapChain = nullptr;
static FrameContext* g_frameContext = nullptr;
static UINT g_numBackBuffers = 0;
static HWND g_hWindow = nullptr;
static bool g_bInitialized = false;
static bool g_bShuttingDown = false;
static std::mutex g_initMutex;

static std::recursive_mutex g_hookMutex;

static void CreateRenderTarget()
{
	if (!g_pSwapChain || !g_pd3dDevice || !g_pd3dRtvDescHeap || !g_frameContext || g_numBackBuffers == 0)
		return;

	SIZE_T rtvDescriptorSize = g_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = g_pd3dRtvDescHeap->GetCPUDescriptorHandleForHeapStart();

	for (UINT i = 0; i < g_numBackBuffers; i++)
	{
		g_frameContext[i].rtvDescriptor = rtvHandle;
		rtvHandle.ptr += rtvDescriptorSize;
	}

	for (UINT i = 0; i < g_numBackBuffers; i++)
	{
		ID3D12Resource* pBackBuffer = nullptr;
		if (SUCCEEDED(g_pSwapChain->GetBuffer(i, IID_PPV_ARGS(&pBackBuffer))))
		{
			g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, g_frameContext[i].rtvDescriptor);
			g_frameContext[i].pBackBuffer = pBackBuffer;
		}
	}
}

static void CleanupRenderTarget()
{
	if (!g_frameContext || g_numBackBuffers == 0)
		return;

	for (UINT i = 0; i < g_numBackBuffers; i++)
	{
		if (g_frameContext[i].pBackBuffer)
		{
			g_frameContext[i].pBackBuffer->Release();
			g_frameContext[i].pBackBuffer = nullptr;
		}
	}
}

static void CleanupDevice() {
	Utils::LogDebug("Called.");

	CleanupRenderTarget();

	if (g_frameContext)
	{
		for (UINT i = 0; i < g_numBackBuffers; i++)
		{
			if (g_frameContext[i].pCommandAllocator)
			{
				g_frameContext[i].pCommandAllocator->Release();
				g_frameContext[i].pCommandAllocator = nullptr;
			}
		}
		delete[] g_frameContext;
		g_frameContext = nullptr;
	}

	if (g_pd3dCommandList)
	{
		g_pd3dCommandList->Release();
		g_pd3dCommandList = nullptr;
	}

	if (g_pd3dSrvDescHeap)
	{
		g_pd3dSrvDescHeap->Release();
		g_pd3dSrvDescHeap = nullptr;
	}
	if (g_pd3dRtvDescHeap)
	{
		g_pd3dRtvDescHeap->Release();
		g_pd3dRtvDescHeap = nullptr;
	}

	g_pd3dCommandQueue = nullptr;
	g_pd3dDevice = nullptr;
	g_pSwapChain = nullptr;
}

static void InitImGui()
{
	if (!ImGui::GetCurrentContext())
		return;

	if (ImGui::GetIO().BackendRendererUserData)
		return;

	if (ImGui::GetIO().BackendPlatformUserData)
		ImGui_ImplWin32_Shutdown();

	ImGui_ImplWin32_Init(g_hWindow);

	ImGui_ImplDX12_InitInfo initInfo{};
	initInfo.Device = g_pd3dDevice;
	initInfo.CommandQueue = g_pd3dCommandQueue;
	initInfo.NumFramesInFlight = g_numBackBuffers;
	initInfo.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	initInfo.SrvDescriptorHeap = g_pd3dSrvDescHeap;

	initInfo.SrvDescriptorAllocFn = [](ImGui_ImplDX12_InitInfo* pInfo, D3D12_CPU_DESCRIPTOR_HANDLE* pOutCpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE* pOutGpuHandle)
	{
		*pOutCpuHandle = pInfo->SrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
		*pOutGpuHandle = pInfo->SrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
	};
	initInfo.SrvDescriptorFreeFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE)
	{
		// nop
	};

	ImGui_ImplDX12_Init(&initInfo);
}

static void RenderImGui(IDXGISwapChain3* pSwapChain)
{
	static bool bInit = false;

	if (!ImGui::GetCurrentContext())
		return;

#if FRAMEWORK_RENDER_DYNAMIC
	if (Framework::renderer->DetectedRenderer == RendererHooks::NONE)
	{
		RendererHooks::ERendererType eExpected = RendererHooks::NONE;
		RendererHooks::ERendererType eDetected = RendererHooks::DetectFromSwapChain(pSwapChain);

		if (Framework::renderer->DetectedRenderer.compare_exchange_strong(eExpected, eDetected))
		{
			Utils::LogDebug(eDetected == RendererHooks::D3D11 ? "Detected D3D11 swap chain (via first real Present)." : "Detected D3D12 swap chain (via first real Present).");
			Framework::renderer->OnRendererDetected(eDetected);
		}
	}

	if (Framework::renderer->DetectedRenderer != RendererHooks::D3D12)
		return;
#endif

	if (g_bInitialized && GUI::bPendingFontRebuild) {
		ImGui::GetIO().Fonts->Clear();
		ImportFonts(GUI::flPendingUIScale);

		ImGui_ImplDX12_CreateDeviceObjects();

		CurrentFont = TahomaFont;
		GUI::bPendingFontRebuild = false;
	}

	if (!bInit)
	{
		std::lock_guard<std::mutex> lock(g_initMutex);

		if (bInit)
			return;

		if (!g_pd3dCommandQueue)
			return;

		if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D12Device), (void**)&g_pd3dDevice)))
		{
			DXGI_SWAP_CHAIN_DESC desc;
			pSwapChain->GetDesc(&desc);
			g_hWindow = desc.OutputWindow;
			g_numBackBuffers = desc.BufferCount;

			if (!g_hWindow || !IsWindow(g_hWindow))
			{
				g_hWindow = Framework::wndproc->hwndWindow;
				Utils::LogDebug("Using Framework window handle for ImGui");
			}

			{
				D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
				heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
				heapDesc.NumDescriptors = 1;
				heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
				if (FAILED(g_pd3dDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&g_pd3dSrvDescHeap))))
				{
					Utils::LogError("Failed to create SRV descriptor heap");
					return;
				}
			}

			{
				D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
				heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
				heapDesc.NumDescriptors = g_numBackBuffers;
				heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
				heapDesc.NodeMask = 1;
				if (FAILED(g_pd3dDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&g_pd3dRtvDescHeap))))
				{
					Utils::LogError("Failed to create RTV descriptor heap");
					return;
				}
			}

			ID3D12CommandAllocator* allocator = nullptr;
			if (FAILED(g_pd3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator))))
			{
				Utils::LogError("Failed to create command allocator");
				return;
			}

			if (FAILED(g_pd3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator, nullptr, IID_PPV_ARGS(&g_pd3dCommandList))))
			{
				allocator->Release();
				Utils::LogError("Failed to create command list");
				return;
			}
			g_pd3dCommandList->Close();

			g_frameContext = new FrameContext[g_numBackBuffers];
			for (UINT i = 0; i < g_numBackBuffers; i++)
			{
				g_frameContext[i].pCommandAllocator = allocator;
			}

			g_pSwapChain = pSwapChain;

			CreateRenderTarget();
			InitImGui();

			bInit = true;
			g_bInitialized = true;
			Utils::LogDebug("DirectX 12 hook initialized successfully");
		}

		return;
	}

	if (!g_pd3dCommandQueue || !g_pd3dDevice || !g_frameContext || !g_pd3dSrvDescHeap)
		return;

	if (g_bShuttingDown)
		return;

	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	ImGui::PushFont(CurrentFont);
	GUI::Render();
	ImGui::PopFont();

	ImGui::Render();

	UINT backBufferIdx = g_pSwapChain->GetCurrentBackBufferIndex();
	FrameContext& frameCtx = g_frameContext[backBufferIdx];

	frameCtx.pCommandAllocator->Reset();

	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = frameCtx.pBackBuffer;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

	ID3D12Fence* pFence = nullptr;
	if (SUCCEEDED(g_pd3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&pFence))))
	{
		HANDLE hEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
		if (hEvent)
		{
			g_pd3dCommandQueue->Signal(pFence, 1);
			pFence->SetEventOnCompletion(1, hEvent);
			WaitForSingleObject(hEvent, 100);
			CloseHandle(hEvent);
		}
		pFence->Release();
	}

	g_pd3dCommandList->Reset(frameCtx.pCommandAllocator, nullptr);
	g_pd3dCommandList->ResourceBarrier(1, &barrier);
	g_pd3dCommandList->OMSetRenderTargets(1, &frameCtx.rtvDescriptor, FALSE, nullptr);
	g_pd3dCommandList->SetDescriptorHeaps(1, &g_pd3dSrvDescHeap);

	ImDrawData* pDrawData = ImGui::GetDrawData();
	if (pDrawData && pDrawData->Valid)
		ImGui_ImplDX12_RenderDrawData(pDrawData, g_pd3dCommandList);

	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	g_pd3dCommandList->ResourceBarrier(1, &barrier);
	g_pd3dCommandList->Close();

	g_pd3dCommandQueue->ExecuteCommandLists(1, (ID3D12CommandList* const*)&g_pd3dCommandList);
}

static Hooking::Hook<HRESULT(WINAPI*)(IDXGISwapChain3*, UINT, UINT)> oPresent;
static HRESULT WINAPI hkPresent(IDXGISwapChain3* pSwapChain, UINT SyncInterval, UINT Flags) {
	std::lock_guard<std::recursive_mutex> lock(g_hookMutex);

	RenderImGui(pSwapChain);

	return oPresent(pSwapChain, SyncInterval, Flags);
}

static Hooking::Hook<HRESULT(WINAPI*)(IDXGISwapChain3*, UINT, UINT, const DXGI_PRESENT_PARAMETERS*)> oPresent1;
static HRESULT WINAPI hkPresent1(IDXGISwapChain3* pSwapChain, UINT SyncInterval, UINT PresentFlags, const DXGI_PRESENT_PARAMETERS* pPresentParameters) {
	std::lock_guard<std::recursive_mutex> lock(g_hookMutex);

	RenderImGui(pSwapChain);

	return oPresent1(pSwapChain, SyncInterval, PresentFlags, pPresentParameters);
}

static Hooking::Hook<HRESULT(WINAPI*)(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT)> oResizeBuffers;
static HRESULT WINAPI hkResizeBuffers(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags) {
	std::lock_guard<std::recursive_mutex> lock(g_hookMutex);

	if (g_bInitialized)
		CleanupRenderTarget();

	g_numBackBuffers = BufferCount;

	HRESULT result = oResizeBuffers(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);

	if (SUCCEEDED(result) && g_bInitialized)
		CreateRenderTarget();

	return result;
}

static Hooking::Hook<HRESULT(WINAPI*)(IDXGISwapChain3*, UINT, UINT, UINT, DXGI_FORMAT, UINT, const UINT*, IUnknown* const*)> oResizeBuffers1;
static HRESULT WINAPI hkResizeBuffers1(IDXGISwapChain3* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags, const UINT* pCreationNodeMask, IUnknown* const* ppPresentQueue) {
	std::lock_guard<std::recursive_mutex> lock(g_hookMutex);

	if (g_bInitialized)
		CleanupRenderTarget();

	g_numBackBuffers = BufferCount;

	HRESULT result = oResizeBuffers1(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags, pCreationNodeMask, ppPresentQueue);

	if (SUCCEEDED(result) && g_bInitialized)
		CreateRenderTarget();

	return result;
}

static Hooking::Hook<void(WINAPI*)(ID3D12CommandQueue*, UINT, ID3D12CommandList* const*)> oExecuteCommandLists;
static void WINAPI hkExecuteCommandLists(ID3D12CommandQueue* pCommandQueue, UINT NumCommandLists, ID3D12CommandList* const* ppCommandLists) {
	std::lock_guard<std::recursive_mutex> lock(g_hookMutex);

	if (!g_pd3dCommandQueue)
		g_pd3dCommandQueue = pCommandQueue;

	return oExecuteCommandLists(pCommandQueue, NumCommandLists, ppCommandLists);
}

static Hooking::Hook<HRESULT(WINAPI*)(IDXGIFactory*, IUnknown*, DXGI_SWAP_CHAIN_DESC*, IDXGISwapChain**)> oCreateSwapChain;
static HRESULT WINAPI hkCreateSwapChain(IDXGIFactory* pFactory, IUnknown* pDevice, DXGI_SWAP_CHAIN_DESC* pDesc, IDXGISwapChain** ppSwapChain) {
	std::lock_guard<std::recursive_mutex> lock(g_hookMutex);

	CleanupRenderTarget();

	return oCreateSwapChain(pFactory, pDevice, pDesc, ppSwapChain);
}

static Hooking::Hook<HRESULT(WINAPI*)(IDXGIFactory*, IUnknown*, HWND, const DXGI_SWAP_CHAIN_DESC1*, const DXGI_SWAP_CHAIN_FULLSCREEN_DESC*, IDXGIOutput*, IDXGISwapChain1**)> oCreateSwapChainForHwnd;
static HRESULT WINAPI hkCreateSwapChainForHwnd(IDXGIFactory* pFactory, IUnknown* pDevice, HWND hWnd, const DXGI_SWAP_CHAIN_DESC1* pDesc, const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* pFullscreenDesc, IDXGIOutput* pRestrictToOutput, IDXGISwapChain1** ppSwapChain) {
	std::lock_guard<std::recursive_mutex> lock(g_hookMutex);

	CleanupRenderTarget();

	return oCreateSwapChainForHwnd(pFactory, pDevice, hWnd, pDesc, pFullscreenDesc, pRestrictToOutput, ppSwapChain);
}

static Hooking::Hook<HRESULT(WINAPI*)(IDXGIFactory*, IUnknown*, IUnknown*, const DXGI_SWAP_CHAIN_DESC1*, IDXGIOutput*, IDXGISwapChain1**)> oCreateSwapChainForCoreWindow;
static HRESULT WINAPI hkCreateSwapChainForCoreWindow(IDXGIFactory* pFactory, IUnknown* pDevice, IUnknown* pWindow, const DXGI_SWAP_CHAIN_DESC1* pDesc, IDXGIOutput* pRestrictToOutput, IDXGISwapChain1** ppSwapChain) {
	std::lock_guard<std::recursive_mutex> lock(g_hookMutex);

	CleanupRenderTarget();

	return oCreateSwapChainForCoreWindow(pFactory, pDevice, pWindow, pDesc, pRestrictToOutput, ppSwapChain);
}

static Hooking::Hook<HRESULT(WINAPI*)(IDXGIFactory*, IUnknown*, const DXGI_SWAP_CHAIN_DESC1*, IDXGIOutput*, IDXGISwapChain1**)> oCreateSwapChainForComposition;
static HRESULT WINAPI hkCreateSwapChainForComposition(IDXGIFactory* pFactory, IUnknown* pDevice, const DXGI_SWAP_CHAIN_DESC1* pDesc, IDXGIOutput* pRestrictToOutput, IDXGISwapChain1** ppSwapChain) {
	std::lock_guard<std::recursive_mutex> lock(g_hookMutex);

	CleanupRenderTarget();

	return oCreateSwapChainForComposition(pFactory, pDevice, pDesc, pRestrictToOutput, ppSwapChain);
}

static bool GetD3D12Addresses(ID3D12Device** ppDevice, ID3D12CommandQueue** ppCommandQueue, IDXGISwapChain3** ppSwapChain, IDXGIFactory4** ppFactory)
{
	WNDCLASSEXW wc = { sizeof(WNDCLASSEXW), CS_CLASSDC, DefWindowProcW, 0L, 0L,
		GetModuleHandleW(nullptr), nullptr, nullptr, nullptr, nullptr, L"DX12Temp", nullptr };

	if (!RegisterClassExW(&wc))
		return false;

	HWND hWnd = CreateWindowW(wc.lpszClassName, L"", WS_OVERLAPPEDWINDOW, 0, 0, 1, 1, nullptr, nullptr, wc.hInstance, nullptr);
	if (!hWnd)
	{
		UnregisterClassW(wc.lpszClassName, wc.hInstance);
		return false;
	}

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = 2;
	swapChainDesc.Width = 1;
	swapChainDesc.Height = 1;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

	ID3D12Device* pTempDevice = nullptr;
	ID3D12CommandQueue* pTempQueue = nullptr;
	IDXGISwapChain1* pTempSwapChain1 = nullptr;
	IDXGISwapChain3* pTempSwapChain = nullptr;
	IDXGIFactory4* pFactory = nullptr;
	IDXGIAdapter* pWarpAdapter = nullptr;
	bool bSuccess = false;

	// This device only exists to read vtable pointers off and gets torn down right after.
	// Use the WARP software adapter instead of the real GPU (falls back to the default
	// hardware adapter if WARP is unavailable) so it can't collide with the target's own
	// D3D12 device creation on the same adapter - when loaded via a proxy DLL this can run
	// at process attach, before the target has touched D3D12 at all, and creating/destroying
	// a real hardware device in that window has been observed to cause an intermittent
	// DXGI_ERROR_DEVICE_REMOVED later on.
	if (SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&pFactory))))
	{
		pFactory->EnumWarpAdapter(IID_PPV_ARGS(&pWarpAdapter));

		if (SUCCEEDED(D3D12CreateDevice(pWarpAdapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&pTempDevice))))
		{
			D3D12_COMMAND_QUEUE_DESC queueDesc = {};
			queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
			queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

			if (SUCCEEDED(pTempDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&pTempQueue))))
			{
				if (SUCCEEDED(pFactory->CreateSwapChainForHwnd(pTempQueue, hWnd, &swapChainDesc, nullptr, nullptr, &pTempSwapChain1)))
				{
					if (SUCCEEDED(pTempSwapChain1->QueryInterface(IID_PPV_ARGS(&pTempSwapChain))))
					{
						*ppDevice = pTempDevice;
						*ppCommandQueue = pTempQueue;
						*ppSwapChain = pTempSwapChain;
						*ppFactory = pFactory;

						bSuccess = true;

						pTempSwapChain1->Release();
					}
					else
					{
						pTempSwapChain1->Release();
					}
				}

				if (!bSuccess)
					pTempQueue->Release();
			}

			if (!bSuccess)
				pTempDevice->Release();
		}

		if (pWarpAdapter)
			pWarpAdapter->Release();

		if (!bSuccess)
			pFactory->Release();
	}

	DestroyWindow(hWnd);
	UnregisterClassW(wc.lpszClassName, wc.hInstance);

	return bSuccess;
}

struct D3D12HookTargets
{
	void* pCreateSwapChain = nullptr;
	void* pCreateSwapChainForHwnd = nullptr;
	void* pCreateSwapChainForCoreWindow = nullptr;
	void* pCreateSwapChainForComposition = nullptr;
	void* pPresent = nullptr;
	void* pResizeBuffers = nullptr;
	void* pPresent1 = nullptr;
	void* pResizeBuffers1 = nullptr;
	void* pExecuteCommandLists = nullptr;
};

static bool ResolveD3D12HookTargets(D3D12HookTargets& out)
{
	ID3D12Device* pTempDevice = nullptr;
	ID3D12CommandQueue* pTempQueue = nullptr;
	IDXGISwapChain3* pTempSwapChain = nullptr;
	IDXGIFactory4* pTempFactory = nullptr;

	if (!GetD3D12Addresses(&pTempDevice, &pTempQueue, &pTempSwapChain, &pTempFactory))
		return false;

	out.pCreateSwapChain = Memory::GetVirtualMethod(pTempFactory, 10);
	out.pCreateSwapChainForHwnd = Memory::GetVirtualMethod(pTempFactory, 15);
	out.pCreateSwapChainForCoreWindow = Memory::GetVirtualMethod(pTempFactory, 16);
	out.pCreateSwapChainForComposition = Memory::GetVirtualMethod(pTempFactory, 24);

	out.pPresent = Memory::GetVirtualMethod(pTempSwapChain, 8);
	out.pResizeBuffers = Memory::GetVirtualMethod(pTempSwapChain, 13);
	out.pPresent1 = Memory::GetVirtualMethod(pTempSwapChain, 22);
	out.pResizeBuffers1 = Memory::GetVirtualMethod(pTempSwapChain, 39);

	out.pExecuteCommandLists = Memory::GetVirtualMethod(pTempQueue, 10);

	pTempSwapChain->Release();
	pTempQueue->Release();
	pTempDevice->Release();
	pTempFactory->Release();

	return out.pCreateSwapChain && out.pCreateSwapChainForHwnd && out.pCreateSwapChainForCoreWindow &&
		out.pCreateSwapChainForComposition && out.pPresent && out.pResizeBuffers && out.pPresent1 &&
		out.pResizeBuffers1 && out.pExecuteCommandLists;
}

bool RendererHooks::D3D12Setup()
{
	Utils::LogDebug("Initializing DirectX 12 hook...");

	D3D12HookTargets targets{};
	const bool bResolvedOk = ResolveD3D12HookTargets(targets);

	if (!bResolvedOk) {
		Utils::LogError("Failed to get DirectX 12 function addresses");
		return false;
	}

	Hooking::HookBatch batch;

	if (!batch.Install(oCreateSwapChain, hkCreateSwapChain, targets.pCreateSwapChain)) {
		Utils::LogHook("CreateSwapChain", oCreateSwapChain.GetStatus());
		return false;
	}

	if (!batch.Install(oCreateSwapChainForHwnd, hkCreateSwapChainForHwnd, targets.pCreateSwapChainForHwnd)) {
		Utils::LogHook("CreateSwapChainForHwnd", oCreateSwapChainForHwnd.GetStatus());
		return false;
	}

	if (!batch.Install(oCreateSwapChainForCoreWindow, hkCreateSwapChainForCoreWindow, targets.pCreateSwapChainForCoreWindow)) {
		Utils::LogHook("CreateSwapChainForCoreWindow", oCreateSwapChainForCoreWindow.GetStatus());
		return false;
	}

	if (!batch.Install(oCreateSwapChainForComposition, hkCreateSwapChainForComposition, targets.pCreateSwapChainForComposition)) {
		Utils::LogHook("CreateSwapChainForComposition", oCreateSwapChainForComposition.GetStatus());
		return false;
	}

	if (!batch.Install(oPresent, hkPresent, targets.pPresent)) {
		Utils::LogHook("Present", oPresent.GetStatus());
		return false;
	}

	if (!batch.Install(oResizeBuffers, hkResizeBuffers, targets.pResizeBuffers)) {
		Utils::LogHook("ResizeBuffers", oResizeBuffers.GetStatus());
		return false;
	}

	if (!batch.Install(oPresent1, hkPresent1, targets.pPresent1)) {
		Utils::LogHook("Present1", oPresent1.GetStatus());
		return false;
	}

	if (!batch.Install(oResizeBuffers1, hkResizeBuffers1, targets.pResizeBuffers1)) {
		Utils::LogHook("ResizeBuffers1", oResizeBuffers1.GetStatus());
		return false;
	}

	if (!batch.Install(oExecuteCommandLists, hkExecuteCommandLists, targets.pExecuteCommandLists)) {
		Utils::LogHook("ExecuteCommandLists", oExecuteCommandLists.GetStatus());
		return false;
	}

	batch.Commit();

	Utils::LogDebug("DirectX 12 hook initialization complete");
	return true;
}

void RendererHooks::D3D12Destroy()
{
	g_bShuttingDown = true;

	{
		std::lock_guard<std::recursive_mutex> lock(g_hookMutex);

		oPresent.Remove();
		oPresent1.Remove();
		oResizeBuffers.Remove();
		oResizeBuffers1.Remove();
		oCreateSwapChain.Remove();
		oCreateSwapChainForHwnd.Remove();
		oCreateSwapChainForCoreWindow.Remove();
		oCreateSwapChainForComposition.Remove();
		oExecuteCommandLists.Remove();
	}

	if (g_bInitialized)
	{
		if (ImGui::GetCurrentContext()) {
			ImGui_ImplDX12_CreateDeviceObjects();

			ImGui_ImplDX12_Shutdown();

			if (ImGui::GetIO().BackendPlatformUserData)
				ImGui_ImplWin32_Shutdown();

			ImGui::DestroyContext();
		}

		CleanupDevice();

		g_bInitialized = false;
	}

	Utils::LogDebug("DirectX 12 hook shutdown complete");
}

#if FRAMEWORK_RENDER_DYNAMIC
void RendererHooks::D3D12ReleaseUnusedResources()
{
	CleanupDevice();
}
#endif

#endif
