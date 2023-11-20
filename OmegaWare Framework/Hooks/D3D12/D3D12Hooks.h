#pragma once
#include "pch.h"
#if FRAMEWORK_RENDER_D3D12

#include <DXGI.h>
#include <D3D12.h>
#include <dxgi1_4.h>
#pragma comment(lib, "d3d12.lib")

#include "../../ImGUI/Styles.h"
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

typedef long(__fastcall* PresentD3D12) (IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
PresentD3D12 oPresent;

typedef void(__fastcall* DrawInstancedD3D12)(ID3D12GraphicsCommandList* dCommandList, UINT VertexCountPerInstance, UINT InstanceCount, UINT StartVertexLocation, UINT StartInstanceLocation);
DrawInstancedD3D12 oDrawInstanced;

typedef void(__fastcall* DrawIndexedInstancedD3D12)(ID3D12GraphicsCommandList* dCommandList, UINT IndexCount, UINT InstanceCount, UINT StartIndex, INT BaseVertex);
DrawIndexedInstancedD3D12 oDrawIndexedInstanced;

void(*oExecuteCommandLists)(ID3D12CommandQueue*, UINT, ID3D12CommandList*);
HRESULT(*oSignal)(ID3D12CommandQueue*, ID3D12Fence*, UINT64);

ID3D12Device* d3d12Device = nullptr;
ID3D12DescriptorHeap* d3d12DescriptorHeapBackBuffers = nullptr;
ID3D12DescriptorHeap* d3d12DescriptorHeapImGuiRender = nullptr;
ID3D12GraphicsCommandList* d3d12CommandList = nullptr;
ID3D12Fence* d3d12Fence = nullptr;
UINT64 d3d12FenceValue = 0;
ID3D12CommandQueue* d3d12CommandQueue = nullptr;

struct __declspec(uuid("189819f1-1db6-4b57-be54-1821339b85f7")) ID3D12Device;

struct FrameContext {
	ID3D12CommandAllocator* commandAllocator = nullptr;
	ID3D12Resource* main_render_target_resource = nullptr;
	D3D12_CPU_DESCRIPTOR_HANDLE main_render_target_descriptor;
};

uint64_t buffersCounts = -1;
FrameContext* frameContext;

HWND Window;
WNDPROC oWndProc;

LRESULT __stdcall WndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam);

	return (GUI::bMenuOpen ? true : CallWindowProcA(oWndProc, hWnd, uMsg, wParam, lParam));
}

bool init = false;
extern long __fastcall hkPresent(IDXGISwapChain3* pSwapChain, UINT SyncInterval, UINT Flags)
{
	if (!Cheat::bShouldRun)
		return oPresent(pSwapChain, SyncInterval, Flags);

	if (!init)
	{
		if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D12Device), (void**)&d3d12Device))) {
			ImGui::CreateContext();

			unsigned char* pixels;
			int width, height;
			ImGuiIO& io = ImGui::GetIO(); (void)io;
			io.ConfigFlags = ImGuiConfigFlags_NoMouseCursorChange;

			SetupStyle();
			ImportFonts();

			io.Fonts->AddFontDefault();
			io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
			//io.IniFilename = NULL;

			CreateEvent(nullptr, false, false, nullptr);

			DXGI_SWAP_CHAIN_DESC sdesc;
			pSwapChain->GetDesc(&sdesc);
			sdesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
			Window = sdesc.OutputWindow;
			sdesc.Windowed = ((GetWindowLongPtrA(Window, GWL_STYLE) & WS_POPUP) != 0) ? false : true;

			buffersCounts = sdesc.BufferCount;
			frameContext = new FrameContext[buffersCounts];

			D3D12_DESCRIPTOR_HEAP_DESC descriptorImGuiRender = {};
			descriptorImGuiRender.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			descriptorImGuiRender.NumDescriptors = buffersCounts;
			descriptorImGuiRender.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

			if (d3d12Device->CreateDescriptorHeap(&descriptorImGuiRender, IID_PPV_ARGS(&d3d12DescriptorHeapImGuiRender)) != S_OK)
				return false;

			ID3D12CommandAllocator* allocator;
			if (d3d12Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator)) != S_OK)
				return false;

			for (size_t i = 0; i < buffersCounts; i++) {
				frameContext[i].commandAllocator = allocator;
			}

			if (d3d12Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator, NULL, IID_PPV_ARGS(&d3d12CommandList)) != S_OK ||
				d3d12CommandList->Close() != S_OK)
				return false;

			D3D12_DESCRIPTOR_HEAP_DESC descriptorBackBuffers;
			descriptorBackBuffers.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
			descriptorBackBuffers.NumDescriptors = buffersCounts;
			descriptorBackBuffers.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			descriptorBackBuffers.NodeMask = 1;

			if (d3d12Device->CreateDescriptorHeap(&descriptorBackBuffers, IID_PPV_ARGS(&d3d12DescriptorHeapBackBuffers)) != S_OK)
				return false;

			const auto rtvDescriptorSize = d3d12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
			D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = d3d12DescriptorHeapBackBuffers->GetCPUDescriptorHandleForHeapStart();

			for (size_t i = 0; i < buffersCounts; i++) {
				ID3D12Resource* pBackBuffer = nullptr;

				frameContext[i].main_render_target_descriptor = rtvHandle;
				pSwapChain->GetBuffer(i, IID_PPV_ARGS(&pBackBuffer));
				d3d12Device->CreateRenderTargetView(pBackBuffer, nullptr, rtvHandle);
				frameContext[i].main_render_target_resource = pBackBuffer;
				rtvHandle.ptr += rtvDescriptorSize;
			}

			ImGui_ImplWin32_Init(Window);
			ImGui_ImplDX12_Init(d3d12Device, buffersCounts,
				DXGI_FORMAT_R8G8B8A8_UNORM, d3d12DescriptorHeapImGuiRender,
				d3d12DescriptorHeapImGuiRender->GetCPUDescriptorHandleForHeapStart(),
				d3d12DescriptorHeapImGuiRender->GetGPUDescriptorHandleForHeapStart());

			ImGui_ImplDX12_CreateDeviceObjects();

			oWndProc = (WNDPROC)SetWindowLongPtrA(Window, GWLP_WNDPROC, (LONG_PTR)WndProc);
		}
		init = true;
	}

	if (!d3d12CommandQueue)
		return oPresent(pSwapChain, SyncInterval, Flags);

	GUI::BeginRender();
	ImGui::PushFont(tahomaFont);
	GUI::Render();
	ImGui::PopFont();

	FrameContext& currentFrameContext = frameContext[pSwapChain->GetCurrentBackBufferIndex()];
	currentFrameContext.commandAllocator->Reset();

	D3D12_RESOURCE_BARRIER barrier;
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = currentFrameContext.main_render_target_resource;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

	d3d12CommandList->Reset(currentFrameContext.commandAllocator, nullptr);
	d3d12CommandList->ResourceBarrier(1, &barrier);
	d3d12CommandList->OMSetRenderTargets(1, &currentFrameContext.main_render_target_descriptor, FALSE, nullptr);
	d3d12CommandList->SetDescriptorHeaps(1, &d3d12DescriptorHeapImGuiRender);

	GUI::EndRender();
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), d3d12CommandList);

	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;

	d3d12CommandList->ResourceBarrier(1, &barrier);
	d3d12CommandList->Close();

	d3d12CommandQueue->ExecuteCommandLists(1, reinterpret_cast<ID3D12CommandList* const*>(&d3d12CommandList));

	return oPresent(pSwapChain, SyncInterval, Flags);
}

void __fastcall hkDrawInstanced(ID3D12GraphicsCommandList* dCommandList, UINT VertexCountPerInstance, UINT InstanceCount, UINT StartVertexLocation, UINT StartInstanceLocation)
{
	return oDrawInstanced(dCommandList, VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation);
}

void __fastcall hkDrawIndexedInstanced(ID3D12GraphicsCommandList* dCommandList, UINT IndexCount, UINT InstanceCount, UINT StartIndex, INT BaseVertex)
{
	return oDrawIndexedInstanced(dCommandList, IndexCount, InstanceCount, StartIndex, BaseVertex);
}

void hkExecuteCommandLists(ID3D12CommandQueue* queue, UINT NumCommandLists, ID3D12CommandList* ppCommandLists)
{
	if (!d3d12CommandQueue)
		d3d12CommandQueue = queue;

	oExecuteCommandLists(queue, NumCommandLists, ppCommandLists);
}

HRESULT hkSignal(ID3D12CommandQueue* queue, ID3D12Fence* fence, UINT64 value)
{
	if (d3d12CommandQueue != nullptr && queue == d3d12CommandQueue) {
		d3d12Fence = fence;
		d3d12FenceValue = value;
	}

	return oSignal(queue, fence, value);
}

void D3D12Release()
{
	d3d12Device->Release();
	d3d12DescriptorHeapBackBuffers->Release();
	d3d12DescriptorHeapImGuiRender->Release();
	d3d12CommandList->Release();
	d3d12Fence->Release();
	d3d12CommandQueue->Release();

	oWndProc = (WNDPROC)SetWindowLongPtrA(Window, GWLP_WNDPROC, (LONG_PTR)oWndProc);
}
#endif