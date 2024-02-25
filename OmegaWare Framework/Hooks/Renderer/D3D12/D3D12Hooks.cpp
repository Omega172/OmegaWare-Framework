#include "pch.h"
#if FRAMEWORK_RENDER_D3D12

static uint64_t* MethodsTable = nullptr;

static ID3D12Device* pD3D12Device = nullptr;
static ID3D12DescriptorHeap* pD3D12DescriptorHeapImGuiRender = nullptr;
static ID3D12DescriptorHeap* pD3D12DescriptorHeapBackBuffers = nullptr;
static ID3D12GraphicsCommandList* pD3D12CommandList = nullptr;

static ID3D12CommandQueue* pD3D12CommandQueue = nullptr;
static ID3D12Fence* pD3D12Fence = nullptr;
UINT64 uiD3D12FenceValue = 0;

struct __declspec(uuid("189819f1-1db6-4b57-be54-1821339b85f7")) ID3D12Device;

struct FrameContext {
	ID3D12CommandAllocator* pCommandAllocator = nullptr;
	ID3D12Resource* pMainRenderTargetResource = nullptr;
	D3D12_CPU_DESCRIPTOR_HANDLE hRenderTargetDescriptor;
};

uint64_t BuffersCounts = -1;
FrameContext* pFrameContext;

static bool CreateDevice(HWND hWnd) {
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

	HMODULE hDXGI;
	HMODULE hD3D12;
	if ((hDXGI = GetModuleHandleA("dxgi.dll")) == NULL || (hD3D12 = GetModuleHandleA("d3d12.dll")) == NULL)
	{
		DestroyWindow(hWindow);
		UnregisterClassA(WndClass.lpszClassName, WndClass.hInstance);
		return false;
	}

	void* pCreateDXGIFactory;
	if ((pCreateDXGIFactory = GetProcAddress(hDXGI, "CreateDXGIFactory")) == NULL)
	{
		DestroyWindow(hWindow);
		UnregisterClassA(WndClass.lpszClassName, WndClass.hInstance);
		return false;
	}

	IDXGIFactory* pFactory;
	if (((long(__stdcall*)(const IID&, void**))(pCreateDXGIFactory))(__uuidof(IDXGIFactory), (void**)&pFactory) < 0)
	{
		DestroyWindow(hWindow);
		UnregisterClassA(WndClass.lpszClassName, WndClass.hInstance);
		return false;
	}

	IDXGIAdapter* pAdapter;
	if (pFactory->EnumAdapters(0, &pAdapter) == DXGI_ERROR_NOT_FOUND)
	{
		DestroyWindow(hWindow);
		UnregisterClassA(WndClass.lpszClassName, WndClass.hInstance);
		return false;
	}

	void* pD3D12CreateDevice;
	if ((pD3D12CreateDevice = GetProcAddress(hD3D12, "D3D12CreateDevice")) == NULL)
	{
		DestroyWindow(hWindow);
		UnregisterClassA(WndClass.lpszClassName, WndClass.hInstance);
		return false;
	}

	ID3D12Device* pDevice;
	if (((long(__stdcall*)(IUnknown*, D3D_FEATURE_LEVEL, const IID&, void**))(pD3D12CreateDevice))(pAdapter, D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), (void**)&pDevice) < 0)
	{
		DestroyWindow(hWindow);
		UnregisterClassA(WndClass.lpszClassName, WndClass.hInstance);
		return false;
	}

	D3D12_COMMAND_QUEUE_DESC QueueDesc;
	QueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	QueueDesc.Priority = 0;
	QueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	QueueDesc.NodeMask = 0;

	ID3D12CommandQueue* pCommandQueue;
	if (pDevice->CreateCommandQueue(&QueueDesc, __uuidof(ID3D12CommandQueue), (void**)&pCommandQueue) < 0)
	{
		DestroyWindow(hWindow);
		UnregisterClassA(WndClass.lpszClassName, WndClass.hInstance);
		return false;
	}

	ID3D12CommandAllocator* pCommandAllocator;
	if (pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, __uuidof(ID3D12CommandAllocator), (void**)&pCommandAllocator) < 0)
	{
		DestroyWindow(hWindow);
		UnregisterClassA(WndClass.lpszClassName, WndClass.hInstance);
		return false;
	}

	ID3D12GraphicsCommandList* pCommandList;
	if (pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, pCommandAllocator, NULL, __uuidof(ID3D12GraphicsCommandList), (void**)&pCommandList) < 0)
	{
		DestroyWindow(hWindow);
		UnregisterClassA(WndClass.lpszClassName, WndClass.hInstance);
		return false;
	}

	DXGI_RATIONAL RefreshRate;
	RefreshRate.Numerator = 60;
	RefreshRate.Denominator = 1;

	DXGI_MODE_DESC BufferDesc;
	BufferDesc.Width = 100;
	BufferDesc.Height = 100;
	BufferDesc.RefreshRate = RefreshRate;
	BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

	DXGI_SAMPLE_DESC SampleDesc;
	SampleDesc.Count = 1;
	SampleDesc.Quality = 0;

	DXGI_SWAP_CHAIN_DESC SwapChainDesc = {};
	SwapChainDesc.BufferDesc = BufferDesc;
	SwapChainDesc.SampleDesc = SampleDesc;
	SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	SwapChainDesc.BufferCount = 2;
	SwapChainDesc.OutputWindow = hWindow;
	SwapChainDesc.Windowed = 1;
	SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	SwapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	IDXGISwapChain* pSwapChain;
	if (pFactory->CreateSwapChain(pCommandQueue, &SwapChainDesc, &pSwapChain) < 0)
	{
		DestroyWindow(hWindow);
		UnregisterClassA(WndClass.lpszClassName, WndClass.hInstance);
		return false;
	}

	MethodsTable = (uint64_t*)calloc(150, sizeof(uint64_t));
	memcpy(MethodsTable, *(uint64_t**)pDevice, 44 * sizeof(uint64_t));
	memcpy(MethodsTable + 44, *(uint64_t**)pCommandQueue, 19 * sizeof(uint64_t));
	memcpy(MethodsTable + 44 + 19, *(uint64_t**)pCommandAllocator, 9 * sizeof(uint64_t));
	memcpy(MethodsTable + 44 + 19 + 9, *(uint64_t**)pCommandList, 60 * sizeof(uint64_t));
	memcpy(MethodsTable + 44 + 19 + 9 + 60, *(uint64_t**)pSwapChain, 18 * sizeof(uint64_t));

	MH_Initialize();

	pDevice->Release();
	pCommandQueue->Release();
	pCommandAllocator->Release();
	pCommandList->Release();
	pSwapChain->Release();

	DestroyWindow(hWindow);

	return true;
}

typedef void(__stdcall* ExecuteCommandLists)(ID3D12CommandQueue* pCommandQueue, UINT NumCommandLists, ID3D12CommandList* const* ppCommandLists);
static ExecuteCommandLists oExecuteCommandLists;
static void hkExecuteCommandLists(ID3D12CommandQueue* pCommandQueue, UINT NumCommandLists, ID3D12CommandList* const* ppCommandLists)
{
	if (!pD3D12CommandQueue)
		pD3D12CommandQueue = pCommandQueue;

	oExecuteCommandLists(pCommandQueue, NumCommandLists, ppCommandLists);
}

typedef HRESULT(__stdcall* SignalD3D12)(ID3D12CommandQueue* pCommandQueue, ID3D12Fence* pFence, UINT64 Value);
static SignalD3D12 oSignalD3D12;
static HRESULT hkSignalD3D12(ID3D12CommandQueue* pCommandQueue, ID3D12Fence* pFence, UINT64 Value)
{
	if (pD3D12CommandQueue && pD3D12CommandQueue == pCommandQueue) {
		pD3D12Fence = pFence;
		uiD3D12FenceValue = Value;
	}

	return oSignalD3D12(pCommandQueue, pFence, Value);
}

typedef HRESULT(__fastcall* PresentD3D12)(IDXGISwapChain3* pSwapChain, UINT SyncInterval, UINT Flags);
static PresentD3D12 oPresentD3D12;
static HRESULT __fastcall hkPresentD3D12(IDXGISwapChain3* pSwapChain, UINT SyncInterval, UINT Flags)
{
	if (!ImGui::GetCurrentContext())
		return oPresentD3D12(pSwapChain, SyncInterval, Flags);

	static std::once_flag init;
	std::call_once(init, [&]() {
		if (!SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D12Device), reinterpret_cast<void**>(&pD3D12Device))))
			return oPresentD3D12(pSwapChain, SyncInterval, Flags);

		CreateEventA(nullptr, FALSE, FALSE, nullptr);

		DXGI_SWAP_CHAIN_DESC sdesc;
		pSwapChain->GetDesc(&sdesc);
		sdesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
		sdesc.OutputWindow = Cheat::wndproc->hwndWindow;
		sdesc.Windowed = ((GetWindowLongPtr(Cheat::wndproc->hwndWindow, GWL_STYLE) & WS_POPUP) != 0) ? false : true;

		BuffersCounts = sdesc.BufferCount;
		pFrameContext = new FrameContext[BuffersCounts];

		D3D12_DESCRIPTOR_HEAP_DESC descriptorImGuiRender = {};
		descriptorImGuiRender.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		descriptorImGuiRender.NumDescriptors = BuffersCounts;
		descriptorImGuiRender.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

		if (pD3D12Device->CreateDescriptorHeap(&descriptorImGuiRender, IID_PPV_ARGS(&pD3D12DescriptorHeapImGuiRender)) != S_OK)
			return oPresentD3D12(pSwapChain, SyncInterval, Flags);

		ID3D12CommandAllocator* pCommandAllocator;
		if (pD3D12Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&pCommandAllocator)) != S_OK)
			return oPresentD3D12(pSwapChain, SyncInterval, Flags);

		for (size_t i = 0; i < BuffersCounts; i++) {
			pFrameContext[i].pCommandAllocator = pCommandAllocator;
		}

		if (pD3D12Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, pCommandAllocator, NULL, IID_PPV_ARGS(&pD3D12CommandList)) != S_OK || pD3D12CommandList->Close() != S_OK)
			return oPresentD3D12(pSwapChain, SyncInterval, Flags);

		D3D12_DESCRIPTOR_HEAP_DESC descriptorBackBuffers;
		descriptorBackBuffers.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		descriptorBackBuffers.NumDescriptors = BuffersCounts;
		descriptorBackBuffers.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		descriptorBackBuffers.NodeMask = 1;

		if (pD3D12Device->CreateDescriptorHeap(&descriptorBackBuffers, IID_PPV_ARGS(&pD3D12DescriptorHeapBackBuffers)) != S_OK)
			return oPresentD3D12(pSwapChain, SyncInterval, Flags);

		const auto rtvDescriptorSize = pD3D12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = pD3D12DescriptorHeapBackBuffers->GetCPUDescriptorHandleForHeapStart();

		for (size_t i = 0; i < BuffersCounts; i++) {
			ID3D12Resource* pBackBuffer = nullptr;

			pFrameContext[i].hRenderTargetDescriptor = rtvHandle;
			pSwapChain->GetBuffer(UINT(i), IID_PPV_ARGS(&pBackBuffer));
			pD3D12Device->CreateRenderTargetView(pBackBuffer, nullptr, rtvHandle);
			pFrameContext[i].pMainRenderTargetResource = pBackBuffer;
			rtvHandle.ptr += rtvDescriptorSize;
		}

		ImGui_ImplDX12_Init(pD3D12Device, BuffersCounts,
			DXGI_FORMAT_R8G8B8A8_UNORM, pD3D12DescriptorHeapBackBuffers,
			pD3D12DescriptorHeapImGuiRender->GetCPUDescriptorHandleForHeapStart(),
			pD3D12DescriptorHeapImGuiRender->GetGPUDescriptorHandleForHeapStart());

		ImGui_ImplDX12_CreateDeviceObjects();
		return oPresentD3D12(pSwapChain, SyncInterval, Flags);
	});

	if (Cheat::bShouldRun)
	{
		if (!pD3D12CommandQueue)
			return oPresentD3D12(pSwapChain, SyncInterval, Flags);

		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		FrameContext& currentFrameContext = pFrameContext[pSwapChain->GetCurrentBackBufferIndex()];
		currentFrameContext.pCommandAllocator->Reset();

		D3D12_RESOURCE_BARRIER barrier;
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = currentFrameContext.pMainRenderTargetResource;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

		pD3D12CommandList->Reset(currentFrameContext.pCommandAllocator, nullptr);
		pD3D12CommandList->ResourceBarrier(1, &barrier);
		pD3D12CommandList->OMSetRenderTargets(1, &currentFrameContext.hRenderTargetDescriptor, FALSE, nullptr);
		pD3D12CommandList->SetDescriptorHeaps(1, &pD3D12DescriptorHeapImGuiRender);

		ImGui::PushFont(CurrentFont);
		GUI::Render();
		ImGui::PopFont();

		ImGui::Render();
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), pD3D12CommandList);

		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;

		pD3D12CommandList->ResourceBarrier(1, &barrier);
		pD3D12CommandList->Close();

		pD3D12CommandQueue->ExecuteCommandLists(1, reinterpret_cast<ID3D12CommandList* const*>(&pD3D12CommandList));
	}

	return oPresentD3D12(pSwapChain, SyncInterval, Flags);
}

typedef void(__fastcall* DrawInstancedD3D12)(ID3D12GraphicsCommandList* pCommandList, UINT VertexCountPerInstance, UINT InstanceCount, UINT StartVertexLocation, UINT StartInstanceLocation);
static DrawInstancedD3D12 oDrawInstancedD3D12;
static void __fastcall hkDrawInstancedD3D12(ID3D12GraphicsCommandList* pCommandList, UINT VertexCountPerInstance, UINT InstanceCount, UINT StartVertexLocation, UINT StartInstanceLocation)
{
	oDrawInstancedD3D12(pCommandList, VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation);
}

typedef void(__fastcall* DrawIndexedInstancedD3D12)(ID3D12GraphicsCommandList* pCommandList, UINT IndexCountPerInstance, UINT InstanceCount, UINT StartIndexLocation, INT BaseVertexLocation, UINT StartInstanceLocation);
static DrawIndexedInstancedD3D12 oDrawIndexedInstancedD3D12;
static void __fastcall hkDrawIndexedInstancedD3D12(ID3D12GraphicsCommandList* pCommandList, UINT IndexCountPerInstance, UINT InstanceCount, UINT StartIndexLocation, INT BaseVertexLocation, UINT StartInstanceLocation)
{
	oDrawIndexedInstancedD3D12(pCommandList, IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
}

bool RendererHooks::Setup()
{
	if (!CreateDevice(Cheat::wndproc->hwndWindow))
	{
		Utils::LogError(Utils::GetLocation(CurrentLoc), "Failed to create DX12 device");
		return false;
	}

	void* pMethod = reinterpret_cast<void*>(MethodsTable[54]);
	if (!pMethod)
	{
		Utils::LogError(Utils::GetLocation(CurrentLoc), "ExecuteCommandLists was null!");
		return false;
	}
	if (MH_CreateHook(pMethod, hkExecuteCommandLists, reinterpret_cast<void**>(&oExecuteCommandLists)) != MH_OK || MH_EnableHook(pMethod) != MH_OK)
	{
		Utils::LogError(Utils::GetLocation(CurrentLoc), "ExecuteCommandLists hook couldnt be created!");
		return false;
	}

	/*
	pMethod = reinterpret_cast<void*>(MethodsTable[58]);
	if (!pMethod)
	{
		Utils::LogError(Utils::GetLocation(CurrentLoc), "SignalD3D12 was null!");
		return false;
	}
	if (MH_CreateHook(pMethod, hkSignalD3D12, reinterpret_cast<void**>(&oSignalD3D12)) != MH_OK || MH_EnableHook(pMethod) != MH_OK)
	{
		Utils::LogError(Utils::GetLocation(CurrentLoc), "SignalD3D12 hook couldnt be created!");
		return false;
	}
	*/

	pMethod = reinterpret_cast<void*>(MethodsTable[140]);
	if (!pMethod)
	{
		Utils::LogError(Utils::GetLocation(CurrentLoc), "PresentD3D12 was null!");
		return false;
	}
	if (MH_CreateHook(pMethod, hkPresentD3D12, reinterpret_cast<void**>(&oPresentD3D12)) != MH_OK || MH_EnableHook(pMethod) != MH_OK)
	{
		Utils::LogError(Utils::GetLocation(CurrentLoc), "PresentD3D12 hook couldnt be created!");
		return false;
	}

	pMethod = reinterpret_cast<void*>(MethodsTable[84]);
	if (!pMethod)
	{
		Utils::LogError(Utils::GetLocation(CurrentLoc), "DrawInstancedD3D12 was null!");
		return false;
	}
	if (MH_CreateHook(pMethod, hkDrawInstancedD3D12, reinterpret_cast<void**>(&oDrawInstancedD3D12)) != MH_OK || MH_EnableHook(pMethod) != MH_OK)
	{
		Utils::LogError(Utils::GetLocation(CurrentLoc), "DrawInstancedD3D12 hook couldnt be created!");
		return false;
	}

	pMethod = reinterpret_cast<void*>(MethodsTable[85]);
	if (!pMethod)
	{
		Utils::LogError(Utils::GetLocation(CurrentLoc), "DrawIndexedInstancedD3D12 was null!");
		return false;
	}
	if (MH_CreateHook(pMethod, hkDrawIndexedInstancedD3D12, reinterpret_cast<void**>(&oDrawIndexedInstancedD3D12)) != MH_OK || MH_EnableHook(pMethod) != MH_OK)
	{
		Utils::LogError(Utils::GetLocation(CurrentLoc), "DrawIndexedInstancedD3D12 hook couldnt be created!");
		return false;
	}

	return true;
}

void RendererHooks::Destroy()
{
	MH_DisableHook(MH_ALL_HOOKS);

	std::this_thread::sleep_for(std::chrono::seconds(3)); // Disable hooks and wait a bit for all threads to finish so we dont crash

	if (ImGui::GetCurrentContext()) {
		ImGuiIO& io = ImGui::GetIO();

		if (io.BackendRendererUserData)
			ImGui_ImplDX12_Shutdown();

		if (io.BackendPlatformUserData)
			ImGui_ImplWin32_Shutdown();

		ImGui::DestroyContext();
	}

	if (pD3D12Device)
		pD3D12Device->Release();

	if (pD3D12DescriptorHeapImGuiRender)
		pD3D12DescriptorHeapImGuiRender->Release();

	if (pD3D12DescriptorHeapBackBuffers)
		pD3D12DescriptorHeapBackBuffers->Release();

	if (pD3D12CommandList)
		pD3D12CommandList->Release();

	if (pD3D12Fence)
		pD3D12Fence->Release();

	if (pD3D12CommandQueue)
		pD3D12CommandQueue->Release();
}

#endif