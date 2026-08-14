#include "pch.h"

#if FRAMEWORK_RENDER_D3D12 || FRAMEWORK_RENDER_DYNAMIC

// Frame context structure
struct FrameContext
{
	ID3D12CommandAllocator* pCommandAllocator = nullptr;
	ID3D12Resource* pBackBuffer = nullptr;
	D3D12_CPU_DESCRIPTOR_HANDLE rtvDescriptor = {};
};

// D3D12 resources
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

// Create render targets
static void CreateRenderTarget()
{
	if (!g_pSwapChain || !g_pd3dDevice || !g_pd3dRtvDescHeap || !g_frameContext || g_numBackBuffers == 0)
		return;

	SIZE_T rtvDescriptorSize = g_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = g_pd3dRtvDescHeap->GetCPUDescriptorHandleForHeapStart();

	// Initialize descriptors
	for (UINT i = 0; i < g_numBackBuffers; i++)
	{
		g_frameContext[i].rtvDescriptor = rtvHandle;
		rtvHandle.ptr += rtvDescriptorSize;
	}

	// Create render target views
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

// Cleanup render targets
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

// Initialize ImGui
static void InitImGui()
{
	if (!ImGui::GetCurrentContext())
		return;

	if (ImGui::GetIO().BackendRendererUserData)
		ImGui_ImplDX12_Shutdown();
	if (ImGui::GetIO().BackendPlatformUserData)
		ImGui_ImplWin32_Shutdown();

	ImGui_ImplWin32_Init(g_hWindow);
	ImGui_ImplDX12_Init(g_pd3dDevice, g_numBackBuffers,
		DXGI_FORMAT_R8G8B8A8_UNORM,
		g_pd3dSrvDescHeap,
		g_pd3dSrvDescHeap->GetCPUDescriptorHandleForHeapStart(),
		g_pd3dSrvDescHeap->GetGPUDescriptorHandleForHeapStart());
}

static void RenderImGui(IDXGISwapChain3* pSwapChain)
{
	static bool bInit = false;

	if (!ImGui::GetCurrentContext())
		return;

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

			// Validate window handle, fallback to Framework window if invalid
			if (!g_hWindow || !IsWindow(g_hWindow))
			{
				g_hWindow = Framework::wndproc->hwndWindow;
				Utils::LogDebug("Using Framework window handle for ImGui");
			}

			// Create SRV descriptor heap for ImGui
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

			// Create RTV descriptor heap
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

			// Create command allocator
			ID3D12CommandAllocator* allocator = nullptr;
			if (FAILED(g_pd3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator))))
			{
				Utils::LogError("Failed to create command allocator");
				return;
			}

			// Create command list
			if (FAILED(g_pd3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator, nullptr, IID_PPV_ARGS(&g_pd3dCommandList))))
			{
				allocator->Release();
				Utils::LogError("Failed to create command list");
				return;
			}
			g_pd3dCommandList->Close();

			// Create frame contexts
			g_frameContext = new FrameContext[g_numBackBuffers];
			for (UINT i = 0; i < g_numBackBuffers; i++)
			{
				g_frameContext[i].pCommandAllocator = allocator;
			}

			g_pSwapChain = pSwapChain;

			// Create render targets
			CreateRenderTarget();

			// Initialize ImGui
			InitImGui();

			bInit = true;
			g_bInitialized = true;
			Utils::LogDebug("DirectX 12 hook initialized successfully");
		}

		return;
	}

	// Check if all required objects exist
	if (!g_pd3dCommandQueue || !g_pd3dDevice || !g_frameContext || !g_pd3dSrvDescHeap)
		return;

	// If shutting down, skip rendering entirely
	if (g_bShuttingDown)
		return;

	// Start new frame
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	// Render ImGui content
	ImGui::PushFont(CurrentFont);
	GUI::Render();
	ImGui::PopFont();

	// Always end the frame (required by ImGui)
	ImGui::Render();

	// Get current back buffer
	UINT backBufferIdx = g_pSwapChain->GetCurrentBackBufferIndex();
	FrameContext& frameCtx = g_frameContext[backBufferIdx];

	// Reset command allocator
	frameCtx.pCommandAllocator->Reset();

	// Prepare resource barriers
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = frameCtx.pBackBuffer;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

	// Wait for GPU to finish previous work to avoid state conflicts
	ID3D12Fence* pFence = nullptr;
	if (SUCCEEDED(g_pd3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&pFence))))
	{
		HANDLE hEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
		if (hEvent)
		{
			g_pd3dCommandQueue->Signal(pFence, 1);
			pFence->SetEventOnCompletion(1, hEvent);
			WaitForSingleObject(hEvent, 100); // Short timeout
			CloseHandle(hEvent);
		}
		pFence->Release();
	}

	// Execute rendering commands
	g_pd3dCommandList->Reset(frameCtx.pCommandAllocator, nullptr);
	g_pd3dCommandList->ResourceBarrier(1, &barrier);
	g_pd3dCommandList->OMSetRenderTargets(1, &frameCtx.rtvDescriptor, FALSE, nullptr);
	g_pd3dCommandList->SetDescriptorHeaps(1, &g_pd3dSrvDescHeap);

	// Render ImGui draw data
	ImDrawData* pDrawData = ImGui::GetDrawData();
	if (pDrawData && pDrawData->Valid)
		ImGui_ImplDX12_RenderDrawData(pDrawData, g_pd3dCommandList);

	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	g_pd3dCommandList->ResourceBarrier(1, &barrier);
	g_pd3dCommandList->Close();

	// Execute command list
	g_pd3dCommandQueue->ExecuteCommandLists(1, (ID3D12CommandList* const*)&g_pd3dCommandList);
}

static Hooking::Hook<HRESULT(WINAPI*)(IDXGISwapChain3*, UINT, UINT)> oPresent;
static HRESULT WINAPI hkPresent(IDXGISwapChain3* pSwapChain, UINT SyncInterval, UINT Flags) {
	RenderImGui(pSwapChain);

	return oPresent(pSwapChain, SyncInterval, Flags);
}

static Hooking::Hook<HRESULT(WINAPI*)(IDXGISwapChain3*, UINT, UINT, const DXGI_PRESENT_PARAMETERS*)> oPresent1;
static HRESULT WINAPI hkPresent1(IDXGISwapChain3* pSwapChain, UINT SyncInterval, UINT PresentFlags, const DXGI_PRESENT_PARAMETERS* pPresentParameters) {
	RenderImGui(pSwapChain);

	return oPresent1(pSwapChain, SyncInterval, PresentFlags, pPresentParameters);
}

static Hooking::Hook<HRESULT(WINAPI*)(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT)> oResizeBuffers;
static HRESULT WINAPI hkResizeBuffers(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags) {
	if (g_bInitialized)
	{
		ImGui_ImplDX12_InvalidateDeviceObjects();
		CleanupRenderTarget();
	}

	g_numBackBuffers = BufferCount;

	HRESULT result = oResizeBuffers(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);

	if (SUCCEEDED(result) && g_bInitialized)
	{
		CreateRenderTarget();
		ImGui_ImplDX12_CreateDeviceObjects();
	}

	return result;
}

static Hooking::Hook<HRESULT(WINAPI*)(IDXGISwapChain3*, UINT, UINT, UINT, DXGI_FORMAT, UINT, const UINT*, IUnknown* const*)> oResizeBuffers1;
static HRESULT WINAPI hkResizeBuffers1(IDXGISwapChain3* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags, const UINT* pCreationNodeMask, IUnknown* const* ppPresentQueue) {
	if (g_bInitialized)
	{
		ImGui_ImplDX12_InvalidateDeviceObjects();
		CleanupRenderTarget();
	}

	g_numBackBuffers = BufferCount;

	HRESULT result = oResizeBuffers1(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags, pCreationNodeMask, ppPresentQueue);

	if (SUCCEEDED(result) && g_bInitialized)
	{
		CreateRenderTarget();
		ImGui_ImplDX12_CreateDeviceObjects();
	}

	return result;
}

static Hooking::Hook<void(WINAPI*)(ID3D12CommandQueue*, UINT, ID3D12CommandList* const*)> oExecuteCommandLists;
static void WINAPI hkExecuteCommandLists(ID3D12CommandQueue* pCommandQueue, UINT NumCommandLists, ID3D12CommandList* const* ppCommandLists) {
	if (!g_pd3dCommandQueue)
		g_pd3dCommandQueue = pCommandQueue;

	return oExecuteCommandLists(pCommandQueue, NumCommandLists, ppCommandLists);
}

static Hooking::Hook<HRESULT(WINAPI*)(IDXGIFactory*, IUnknown*, DXGI_SWAP_CHAIN_DESC*, IDXGISwapChain**)> oCreateSwapChain;
static HRESULT WINAPI hkCreateSwapChain(IDXGIFactory* pFactory, IUnknown* pDevice, DXGI_SWAP_CHAIN_DESC* pDesc, IDXGISwapChain** ppSwapChain) {
	CleanupRenderTarget();

	return oCreateSwapChain(pFactory, pDevice, pDesc, ppSwapChain);
}

static Hooking::Hook<HRESULT(WINAPI*)(IDXGIFactory*, IUnknown*, HWND, const DXGI_SWAP_CHAIN_DESC1*, const DXGI_SWAP_CHAIN_FULLSCREEN_DESC*, IDXGIOutput*, IDXGISwapChain1**)> oCreateSwapChainForHwnd;
static HRESULT WINAPI hkCreateSwapChainForHwnd(IDXGIFactory* pFactory, IUnknown* pDevice, HWND hWnd, const DXGI_SWAP_CHAIN_DESC1* pDesc, const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* pFullscreenDesc, IDXGIOutput* pRestrictToOutput, IDXGISwapChain1** ppSwapChain) {
	CleanupRenderTarget();

	return oCreateSwapChainForHwnd(pFactory, pDevice, hWnd, pDesc, pFullscreenDesc, pRestrictToOutput, ppSwapChain);
}

static Hooking::Hook<HRESULT(WINAPI*)(IDXGIFactory*, IUnknown*, IUnknown*, const DXGI_SWAP_CHAIN_DESC1*, IDXGIOutput*, IDXGISwapChain1**)> oCreateSwapChainForCoreWindow;
static HRESULT WINAPI hkCreateSwapChainForCoreWindow(IDXGIFactory* pFactory, IUnknown* pDevice, IUnknown* pWindow, const DXGI_SWAP_CHAIN_DESC1* pDesc, IDXGIOutput* pRestrictToOutput, IDXGISwapChain1** ppSwapChain) {
	CleanupRenderTarget();

	return oCreateSwapChainForCoreWindow(pFactory, pDevice, pWindow, pDesc, pRestrictToOutput, ppSwapChain);
}

static Hooking::Hook<HRESULT(WINAPI*)(IDXGIFactory*, IUnknown*, const DXGI_SWAP_CHAIN_DESC1*, IDXGIOutput*, IDXGISwapChain1**)> oCreateSwapChainForComposition;
static HRESULT WINAPI hkCreateSwapChainForComposition(IDXGIFactory* pFactory, IUnknown* pDevice, const DXGI_SWAP_CHAIN_DESC1* pDesc, IDXGIOutput* pRestrictToOutput, IDXGISwapChain1** ppSwapChain) {
	CleanupRenderTarget();

	return oCreateSwapChainForComposition(pFactory, pDevice, pDesc, pRestrictToOutput, ppSwapChain);
}

// Get D3D12 function addresses via vtable
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
	bool bSuccess = false;

	if (SUCCEEDED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&pTempDevice))))
	{
		D3D12_COMMAND_QUEUE_DESC queueDesc = {};
		queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

		if (SUCCEEDED(pTempDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&pTempQueue))))
		{
			if (SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&pFactory))))
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
					pFactory->Release();
			}

			if (!bSuccess)
				pTempQueue->Release();
		}

		if (!bSuccess)
			pTempDevice->Release();
	}

	DestroyWindow(hWnd);
	UnregisterClassW(wc.lpszClassName, wc.hInstance);

	return bSuccess;
}

// Resolves the vtable slots we need to hook using a throwaway device/swapchain/queue/factory,
// then releases all of them immediately. Hook installation itself never touches these COM
// objects, so a failed install can never leak them.
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
	if (!ResolveD3D12HookTargets(targets)) {
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
	Sleep(100); // Give Present a chance to see shutdown flag

	if (g_bInitialized)
	{
		if (ImGui::GetCurrentContext()) {
			if (ImGui::GetIO().BackendRendererUserData)
				ImGui_ImplDX12_Shutdown();

			if (ImGui::GetIO().BackendPlatformUserData)
				ImGui_ImplWin32_Shutdown();

			ImGui::DestroyContext();
		}

		CleanupDevice();

		g_bInitialized = false;
	}

	oPresent.Remove();
	oPresent1.Remove();
	oResizeBuffers.Remove();
	oResizeBuffers1.Remove();
	oCreateSwapChain.Remove();
	oCreateSwapChainForHwnd.Remove();
	oCreateSwapChainForCoreWindow.Remove();
	oCreateSwapChainForComposition.Remove();
	oExecuteCommandLists.Remove();

	Utils::LogDebug("DirectX 12 hook shutdown complete");
}

#endif
