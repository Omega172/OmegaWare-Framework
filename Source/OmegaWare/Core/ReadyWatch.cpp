#include "Core/ReadyWatch.hpp"
#include "Core/Framework.hpp"
#include "Core/Log.hpp"

#include "OmegaRenderHook/Hook/InlineHook.hpp"
#include "OmegaRenderHook/Render/Pipeline.hpp"

#include <Windows.h>
#include <d3d11.h>
#include <d3d9.h>
#include <dxgi1_2.h>
#include <wrl/client.h>

#include <array>
#include <atomic>
#include <mutex>
#include <thread>

#ifndef OMEGAWARE_GAME_DEPTH
#define OMEGAWARE_GAME_DEPTH 1
#endif

namespace OmegaWare::ReadyWatch
{
	namespace
	{
		template <typename T>
		using ComPtr = Microsoft::WRL::ComPtr<T>;
		using namespace std::chrono_literals;

		constexpr std::size_t uTargets = 4;
		constexpr int iReadyPresents = 60;
		constexpr std::int64_t iReadyMs = 2000;
		constexpr std::int64_t iVulkanSettleMs = 3000;
		constexpr std::int64_t iFallbackMs = 60000;
		constexpr std::int64_t iMissedStartMs = 5000;

		enum class EApi : std::uint8_t
		{
			None,
			DX9,
			DXGI,
			OpenGL,
			Vulkan,
		};

		constexpr std::array<std::string_view, 5> kApiNames = { "none", "Direct3D 9", "DXGI", "OpenGL", "Vulkan" };

		enum EKind : std::size_t
		{
			kPresent,
			kPresent1,
			kCreateSwapChain,
			kCreateSwapChainForHwnd,
			kCreateSwapChainForCoreWindow,
			kCreateSwapChainForComposition,
			kCreateDevice9,
			kCreateDevice9Ex,
			kPresent9,
			kPresent9Ex,
			kKindCount,
		};

		enum EExport : std::size_t
		{
			kCreateDXGIFactory,
			kCreateDXGIFactory1,
			kCreateDXGIFactory2,
			kD3D11CreateDevice,
			kD3D11CreateDeviceAndSwapChain,
			kDirect3DCreate9,
			kDirect3DCreate9Ex,
			kWglSwapBuffers,
			kVkCreateDevice,
			kVkQueuePresent,
			kExportCount,
		};

		struct stGroup
		{
			std::array<OmegaRenderHook::clInlineHook, uTargets> aHooks;
			std::array<void*, uTargets> aTargets{};
			std::size_t uCount = 0;
		};

		struct stExport
		{
			const wchar_t* szModule = nullptr;
			const char* szName = nullptr;
			void* pDetour = nullptr;
			bool bDone = false;
			OmegaRenderHook::clInlineHook clHook;
		};

		struct stState
		{
			std::recursive_mutex mtxHooks;
			std::array<stGroup, kKindCount> aGroups;
			std::array<stExport, kExportCount> aExports;
			std::atomic<int> iPresents = 0;
			std::atomic<int> iCalls = 0;
			std::atomic<EApi> eApi = EApi::None;
			std::atomic<std::int64_t> iFirstPresentMs = 0;
			std::atomic<std::int64_t> iVulkanDeviceMs = 0;
			std::atomic<bool> bStopped = false;
			std::atomic<bool> bSawCreation = false;
			std::atomic<bool> bPolling = false;
			std::thread thPoll;
		};

		stState& State()
		{
			static stState* s_pState = new stState();
			return *s_pState;
		}

		class clCallScope
		{
		public:
			clCallScope() { State().iCalls.fetch_add(1, std::memory_order_acq_rel); }
			~clCallScope() { State().iCalls.fetch_sub(1, std::memory_order_acq_rel); }

			clCallScope(const clCallScope&) = delete;
			clCallScope& operator=(const clCallScope&) = delete;
		};

		[[nodiscard]] std::int64_t NowMs()
		{
			return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
		}

		[[nodiscard]] void** VTableOf(void* pObject)
		{
			return *static_cast<void***>(pObject);
		}

		void NoteCreation()
		{
			State().bSawCreation.store(true, std::memory_order_release);
		}

		void CountPresent(EApi eApi)
		{
			stState& rState = State();
			rState.bSawCreation.store(true, std::memory_order_release);
			if (rState.bStopped.load(std::memory_order_relaxed))
				return;
			EApi eExpected = EApi::None;
			rState.eApi.compare_exchange_strong(eExpected, eApi);
			if (rState.iPresents.fetch_add(1, std::memory_order_acq_rel) == 0)
				rState.iFirstPresentMs = NowMs();
		}

		void HookMethod(EKind eKind, void* pTarget, const std::array<void*, uTargets>& aDetours)
		{
			stState& rState = State();
			std::scoped_lock lckHooks(rState.mtxHooks);
			stGroup& rGroup = rState.aGroups[eKind];
			if (!pTarget || rState.bStopped || rGroup.uCount == uTargets || std::ranges::find(rGroup.aTargets, pTarget) != rGroup.aTargets.end())
				return;
			if (rGroup.aHooks[rGroup.uCount].Install(pTarget, aDetours[rGroup.uCount]))
				rGroup.aTargets[rGroup.uCount++] = pTarget;
		}

		template <EKind eKind, std::size_t uIndex, typename Fn>
		[[nodiscard]] Fn Original()
		{
			return State().aGroups[eKind].aHooks[uIndex].template Original<Fn>();
		}

		template <EExport eExport, typename Fn>
		[[nodiscard]] Fn OriginalExport()
		{
			return State().aExports[eExport].clHook.template Original<Fn>();
		}

#define OMEGAWARE_READY_DETOURS(Name) \
		std::array<void*, uTargets>{ reinterpret_cast<void*>(&Name<0>), reinterpret_cast<void*>(&Name<1>), reinterpret_cast<void*>(&Name<2>), reinterpret_cast<void*>(&Name<3>) }

		using PresentFn = HRESULT(STDMETHODCALLTYPE*)(IDXGISwapChain*, UINT, UINT);
		using Present1Fn = HRESULT(STDMETHODCALLTYPE*)(IDXGISwapChain1*, UINT, UINT, const DXGI_PRESENT_PARAMETERS*);
		using CreateSwapChainFn = HRESULT(STDMETHODCALLTYPE*)(IDXGIFactory*, IUnknown*, DXGI_SWAP_CHAIN_DESC*, IDXGISwapChain**);
		using CreateSwapChainForHwndFn = HRESULT(STDMETHODCALLTYPE*)(IDXGIFactory2*, IUnknown*, HWND, const DXGI_SWAP_CHAIN_DESC1*, const DXGI_SWAP_CHAIN_FULLSCREEN_DESC*, IDXGIOutput*, IDXGISwapChain1**);
		using CreateSwapChainForCoreWindowFn = HRESULT(STDMETHODCALLTYPE*)(IDXGIFactory2*, IUnknown*, IUnknown*, const DXGI_SWAP_CHAIN_DESC1*, IDXGIOutput*, IDXGISwapChain1**);
		using CreateSwapChainForCompositionFn = HRESULT(STDMETHODCALLTYPE*)(IDXGIFactory2*, IUnknown*, const DXGI_SWAP_CHAIN_DESC1*, IDXGIOutput*, IDXGISwapChain1**);
		using CreateDevice9Fn = HRESULT(STDMETHODCALLTYPE*)(IDirect3D9*, UINT, D3DDEVTYPE, HWND, DWORD, D3DPRESENT_PARAMETERS*, IDirect3DDevice9**);
		using CreateDevice9ExFn = HRESULT(STDMETHODCALLTYPE*)(IDirect3D9Ex*, UINT, D3DDEVTYPE, HWND, DWORD, D3DPRESENT_PARAMETERS*, D3DDISPLAYMODEEX*, IDirect3DDevice9Ex**);
		using Present9Fn = HRESULT(STDMETHODCALLTYPE*)(IDirect3DDevice9*, const RECT*, const RECT*, HWND, const RGNDATA*);
		using Present9ExFn = HRESULT(STDMETHODCALLTYPE*)(IDirect3DDevice9Ex*, const RECT*, const RECT*, HWND, const RGNDATA*, DWORD);

		template <std::size_t I>
		HRESULT STDMETHODCALLTYPE DetourPresent(IDXGISwapChain* pSwapChain, UINT uSync, UINT uFlags)
		{
			clCallScope clScope;
			if (!(uFlags & DXGI_PRESENT_TEST))
				CountPresent(EApi::DXGI);
			return Original<kPresent, I, PresentFn>()(pSwapChain, uSync, uFlags);
		}

		template <std::size_t I>
		HRESULT STDMETHODCALLTYPE DetourPresent1(IDXGISwapChain1* pSwapChain, UINT uSync, UINT uFlags, const DXGI_PRESENT_PARAMETERS* pParameters)
		{
			clCallScope clScope;
			if (!(uFlags & DXGI_PRESENT_TEST))
				CountPresent(EApi::DXGI);
			return Original<kPresent1, I, Present1Fn>()(pSwapChain, uSync, uFlags, pParameters);
		}

		void HookSwapChain(IUnknown* pSwapChain)
		{
			if (!pSwapChain)
				return;
			HookMethod(kPresent, VTableOf(pSwapChain)[8], OMEGAWARE_READY_DETOURS(DetourPresent));
			ComPtr<IDXGISwapChain1> pSwapChain1;
			if (SUCCEEDED(pSwapChain->QueryInterface(IID_PPV_ARGS(&pSwapChain1))))
				HookMethod(kPresent1, VTableOf(pSwapChain1.Get())[22], OMEGAWARE_READY_DETOURS(DetourPresent1));
		}

		template <std::size_t I>
		HRESULT STDMETHODCALLTYPE DetourCreateSwapChain(IDXGIFactory* pFactory, IUnknown* pDevice, DXGI_SWAP_CHAIN_DESC* pDesc, IDXGISwapChain** ppSwapChain)
		{
			clCallScope clScope;
			const HRESULT hResult = Original<kCreateSwapChain, I, CreateSwapChainFn>()(pFactory, pDevice, pDesc, ppSwapChain);
			if (SUCCEEDED(hResult) && ppSwapChain)
				HookSwapChain(*ppSwapChain);
			return hResult;
		}

		template <std::size_t I>
		HRESULT STDMETHODCALLTYPE DetourCreateSwapChainForHwnd(IDXGIFactory2* pFactory, IUnknown* pDevice, HWND hWindow, const DXGI_SWAP_CHAIN_DESC1* pDesc, const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* pFullscreen, IDXGIOutput* pOutput, IDXGISwapChain1** ppSwapChain)
		{
			clCallScope clScope;
			const HRESULT hResult = Original<kCreateSwapChainForHwnd, I, CreateSwapChainForHwndFn>()(pFactory, pDevice, hWindow, pDesc, pFullscreen, pOutput, ppSwapChain);
			if (SUCCEEDED(hResult) && ppSwapChain)
				HookSwapChain(*ppSwapChain);
			return hResult;
		}

		template <std::size_t I>
		HRESULT STDMETHODCALLTYPE DetourCreateSwapChainForCoreWindow(IDXGIFactory2* pFactory, IUnknown* pDevice, IUnknown* pWindow, const DXGI_SWAP_CHAIN_DESC1* pDesc, IDXGIOutput* pOutput, IDXGISwapChain1** ppSwapChain)
		{
			clCallScope clScope;
			const HRESULT hResult = Original<kCreateSwapChainForCoreWindow, I, CreateSwapChainForCoreWindowFn>()(pFactory, pDevice, pWindow, pDesc, pOutput, ppSwapChain);
			if (SUCCEEDED(hResult) && ppSwapChain)
				HookSwapChain(*ppSwapChain);
			return hResult;
		}

		template <std::size_t I>
		HRESULT STDMETHODCALLTYPE DetourCreateSwapChainForComposition(IDXGIFactory2* pFactory, IUnknown* pDevice, const DXGI_SWAP_CHAIN_DESC1* pDesc, IDXGIOutput* pOutput, IDXGISwapChain1** ppSwapChain)
		{
			clCallScope clScope;
			const HRESULT hResult = Original<kCreateSwapChainForComposition, I, CreateSwapChainForCompositionFn>()(pFactory, pDevice, pDesc, pOutput, ppSwapChain);
			if (SUCCEEDED(hResult) && ppSwapChain)
				HookSwapChain(*ppSwapChain);
			return hResult;
		}

		void HookFactory(IUnknown* pFactory)
		{
			if (!pFactory)
				return;
			HookMethod(kCreateSwapChain, VTableOf(pFactory)[10], OMEGAWARE_READY_DETOURS(DetourCreateSwapChain));
			ComPtr<IDXGIFactory2> pFactory2;
			if (FAILED(pFactory->QueryInterface(IID_PPV_ARGS(&pFactory2))))
				return;
			void** ppVTable = VTableOf(pFactory2.Get());
			HookMethod(kCreateSwapChainForHwnd, ppVTable[15], OMEGAWARE_READY_DETOURS(DetourCreateSwapChainForHwnd));
			HookMethod(kCreateSwapChainForCoreWindow, ppVTable[16], OMEGAWARE_READY_DETOURS(DetourCreateSwapChainForCoreWindow));
			HookMethod(kCreateSwapChainForComposition, ppVTable[24], OMEGAWARE_READY_DETOURS(DetourCreateSwapChainForComposition));
		}

		void HookFactoryOfDevice(IUnknown* pDevice)
		{
			ComPtr<IDXGIDevice> pDxgiDevice;
			ComPtr<IDXGIAdapter> pAdapter;
			ComPtr<IDXGIFactory> pFactory;
			if (pDevice && SUCCEEDED(pDevice->QueryInterface(IID_PPV_ARGS(&pDxgiDevice))) && SUCCEEDED(pDxgiDevice->GetAdapter(&pAdapter)) && SUCCEEDED(pAdapter->GetParent(IID_PPV_ARGS(&pFactory))))
				HookFactory(pFactory.Get());
		}

		template <std::size_t I>
		HRESULT STDMETHODCALLTYPE DetourPresent9(IDirect3DDevice9* pDevice, const RECT* pSource, const RECT* pDest, HWND hWindow, const RGNDATA* pDirty)
		{
			clCallScope clScope;
			CountPresent(EApi::DX9);
			return Original<kPresent9, I, Present9Fn>()(pDevice, pSource, pDest, hWindow, pDirty);
		}

		template <std::size_t I>
		HRESULT STDMETHODCALLTYPE DetourPresent9Ex(IDirect3DDevice9Ex* pDevice, const RECT* pSource, const RECT* pDest, HWND hWindow, const RGNDATA* pDirty, DWORD uFlags)
		{
			clCallScope clScope;
			CountPresent(EApi::DX9);
			return Original<kPresent9Ex, I, Present9ExFn>()(pDevice, pSource, pDest, hWindow, pDirty, uFlags);
		}

		void HookDevice9(IDirect3DDevice9* pDevice)
		{
			if (!pDevice)
				return;
			HookMethod(kPresent9, VTableOf(pDevice)[17], OMEGAWARE_READY_DETOURS(DetourPresent9));
			ComPtr<IDirect3DDevice9Ex> pDeviceEx;
			if (SUCCEEDED(pDevice->QueryInterface(IID_PPV_ARGS(&pDeviceEx))))
				HookMethod(kPresent9Ex, VTableOf(pDeviceEx.Get())[121], OMEGAWARE_READY_DETOURS(DetourPresent9Ex));
		}

		template <std::size_t I>
		HRESULT STDMETHODCALLTYPE DetourCreateDevice9(IDirect3D9* pDirect3D, UINT uAdapter, D3DDEVTYPE eType, HWND hFocus, DWORD uFlags, D3DPRESENT_PARAMETERS* pParameters, IDirect3DDevice9** ppDevice)
		{
			clCallScope clScope;
			const HRESULT hResult = Original<kCreateDevice9, I, CreateDevice9Fn>()(pDirect3D, uAdapter, eType, hFocus, uFlags, pParameters, ppDevice);
			if (SUCCEEDED(hResult) && ppDevice)
				HookDevice9(*ppDevice);
			return hResult;
		}

		template <std::size_t I>
		HRESULT STDMETHODCALLTYPE DetourCreateDevice9Ex(IDirect3D9Ex* pDirect3D, UINT uAdapter, D3DDEVTYPE eType, HWND hFocus, DWORD uFlags, D3DPRESENT_PARAMETERS* pParameters, D3DDISPLAYMODEEX* pMode, IDirect3DDevice9Ex** ppDevice)
		{
			clCallScope clScope;
			const HRESULT hResult = Original<kCreateDevice9Ex, I, CreateDevice9ExFn>()(pDirect3D, uAdapter, eType, hFocus, uFlags, pParameters, pMode, ppDevice);
			if (SUCCEEDED(hResult) && ppDevice)
				HookDevice9(*ppDevice);
			return hResult;
		}

		void HookDirect3D9(IDirect3D9* pDirect3D, bool bEx)
		{
			if (!pDirect3D)
				return;
			HookMethod(kCreateDevice9, VTableOf(pDirect3D)[16], OMEGAWARE_READY_DETOURS(DetourCreateDevice9));
			if (bEx)
				HookMethod(kCreateDevice9Ex, VTableOf(pDirect3D)[20], OMEGAWARE_READY_DETOURS(DetourCreateDevice9Ex));
		}

		HRESULT WINAPI DetourCreateDXGIFactory(REFIID rIid, void** ppFactory)
		{
			clCallScope clScope;
			const HRESULT hResult = OriginalExport<kCreateDXGIFactory, decltype(&DetourCreateDXGIFactory)>()(rIid, ppFactory);
			if (SUCCEEDED(hResult) && ppFactory)
			{
				NoteCreation();
				HookFactory(static_cast<IUnknown*>(*ppFactory));
			}
			return hResult;
		}

		HRESULT WINAPI DetourCreateDXGIFactory1(REFIID rIid, void** ppFactory)
		{
			clCallScope clScope;
			const HRESULT hResult = OriginalExport<kCreateDXGIFactory1, decltype(&DetourCreateDXGIFactory1)>()(rIid, ppFactory);
			if (SUCCEEDED(hResult) && ppFactory)
			{
				NoteCreation();
				HookFactory(static_cast<IUnknown*>(*ppFactory));
			}
			return hResult;
		}

		HRESULT WINAPI DetourCreateDXGIFactory2(UINT uFlags, REFIID rIid, void** ppFactory)
		{
			clCallScope clScope;
			const HRESULT hResult = OriginalExport<kCreateDXGIFactory2, decltype(&DetourCreateDXGIFactory2)>()(uFlags, rIid, ppFactory);
			if (SUCCEEDED(hResult) && ppFactory)
			{
				NoteCreation();
				HookFactory(static_cast<IUnknown*>(*ppFactory));
			}
			return hResult;
		}

		HRESULT WINAPI DetourD3D11CreateDevice(IDXGIAdapter* pAdapter, D3D_DRIVER_TYPE eDriver, HMODULE hSoftware, UINT uFlags, const D3D_FEATURE_LEVEL* pLevels, UINT uLevels, UINT uVersion, ID3D11Device** ppDevice, D3D_FEATURE_LEVEL* pLevel, ID3D11DeviceContext** ppContext)
		{
			clCallScope clScope;
			const HRESULT hResult = OriginalExport<kD3D11CreateDevice, decltype(&DetourD3D11CreateDevice)>()(pAdapter, eDriver, hSoftware, uFlags, pLevels, uLevels, uVersion, ppDevice, pLevel, ppContext);
			if (SUCCEEDED(hResult) && ppDevice)
			{
				NoteCreation();
				HookFactoryOfDevice(*ppDevice);
			}
			return hResult;
		}

		HRESULT WINAPI DetourD3D11CreateDeviceAndSwapChain(IDXGIAdapter* pAdapter, D3D_DRIVER_TYPE eDriver, HMODULE hSoftware, UINT uFlags, const D3D_FEATURE_LEVEL* pLevels, UINT uLevels, UINT uVersion, const DXGI_SWAP_CHAIN_DESC* pDesc, IDXGISwapChain** ppSwapChain, ID3D11Device** ppDevice, D3D_FEATURE_LEVEL* pLevel, ID3D11DeviceContext** ppContext)
		{
			clCallScope clScope;
			const HRESULT hResult = OriginalExport<kD3D11CreateDeviceAndSwapChain, decltype(&DetourD3D11CreateDeviceAndSwapChain)>()(pAdapter, eDriver, hSoftware, uFlags, pLevels, uLevels, uVersion, pDesc, ppSwapChain, ppDevice, pLevel, ppContext);
			if (SUCCEEDED(hResult))
			{
				NoteCreation();
				if (ppSwapChain)
					HookSwapChain(*ppSwapChain);
				if (ppDevice)
					HookFactoryOfDevice(*ppDevice);
			}
			return hResult;
		}

		IDirect3D9* WINAPI DetourDirect3DCreate9(UINT uVersion)
		{
			clCallScope clScope;
			IDirect3D9* pDirect3D = OriginalExport<kDirect3DCreate9, decltype(&DetourDirect3DCreate9)>()(uVersion);
			if (pDirect3D)
				NoteCreation();
			HookDirect3D9(pDirect3D, false);
			return pDirect3D;
		}

		HRESULT WINAPI DetourDirect3DCreate9Ex(UINT uVersion, IDirect3D9Ex** ppDirect3D)
		{
			clCallScope clScope;
			const HRESULT hResult = OriginalExport<kDirect3DCreate9Ex, decltype(&DetourDirect3DCreate9Ex)>()(uVersion, ppDirect3D);
			if (SUCCEEDED(hResult) && ppDirect3D)
			{
				NoteCreation();
				HookDirect3D9(*ppDirect3D, true);
			}
			return hResult;
		}

		BOOL WINAPI DetourWglSwapBuffers(HDC hDC)
		{
			clCallScope clScope;
			CountPresent(EApi::OpenGL);
			return OriginalExport<kWglSwapBuffers, decltype(&DetourWglSwapBuffers)>()(hDC);
		}

		std::int32_t WINAPI DetourVkCreateDevice(void* pPhysical, const void* pInfo, const void* pAllocator, void** ppDevice)
		{
			clCallScope clScope;
			const std::int32_t iResult = OriginalExport<kVkCreateDevice, decltype(&DetourVkCreateDevice)>()(pPhysical, pInfo, pAllocator, ppDevice);
			if (iResult == 0 && !State().bStopped)
			{
				EApi eExpected = EApi::None;
				State().eApi.compare_exchange_strong(eExpected, EApi::Vulkan);
				State().iVulkanDeviceMs = NowMs();
				NoteCreation();
			}
			if (iResult == 0 && ppDevice && *ppDevice)
				OmegaRenderHook::Pipeline::NoteVulkanDevice(*ppDevice, pPhysical);
			return iResult;
		}

		std::int32_t WINAPI DetourVkQueuePresent(void* pQueue, const void* pInfo)
		{
			clCallScope clScope;
			CountPresent(EApi::Vulkan);
			return OriginalExport<kVkQueuePresent, decltype(&DetourVkQueuePresent)>()(pQueue, pInfo);
		}

		void InitializeExports(stState& rState)
		{
			const auto fnSet = [&](EExport eExport, const wchar_t* szModule, const char* szName, void* pDetour)
			{
				rState.aExports[eExport].szModule = szModule;
				rState.aExports[eExport].szName = szName;
				rState.aExports[eExport].pDetour = pDetour;
			};
			fnSet(kCreateDXGIFactory, L"dxgi.dll", "CreateDXGIFactory", reinterpret_cast<void*>(&DetourCreateDXGIFactory));
			fnSet(kCreateDXGIFactory1, L"dxgi.dll", "CreateDXGIFactory1", reinterpret_cast<void*>(&DetourCreateDXGIFactory1));
			fnSet(kCreateDXGIFactory2, L"dxgi.dll", "CreateDXGIFactory2", reinterpret_cast<void*>(&DetourCreateDXGIFactory2));
			fnSet(kD3D11CreateDevice, L"d3d11.dll", "D3D11CreateDevice", reinterpret_cast<void*>(&DetourD3D11CreateDevice));
			fnSet(kD3D11CreateDeviceAndSwapChain, L"d3d11.dll", "D3D11CreateDeviceAndSwapChain", reinterpret_cast<void*>(&DetourD3D11CreateDeviceAndSwapChain));
			fnSet(kDirect3DCreate9, L"d3d9.dll", "Direct3DCreate9", reinterpret_cast<void*>(&DetourDirect3DCreate9));
			fnSet(kDirect3DCreate9Ex, L"d3d9.dll", "Direct3DCreate9Ex", reinterpret_cast<void*>(&DetourDirect3DCreate9Ex));
			fnSet(kWglSwapBuffers, L"opengl32.dll", "wglSwapBuffers", reinterpret_cast<void*>(&DetourWglSwapBuffers));
			fnSet(kVkCreateDevice, L"vulkan-1.dll", "vkCreateDevice", reinterpret_cast<void*>(&DetourVkCreateDevice));
			fnSet(kVkQueuePresent, L"vulkan-1.dll", "vkQueuePresentKHR", reinterpret_cast<void*>(&DetourVkQueuePresent));
		}

		[[nodiscard]] bool PollExports(stState& rState)
		{
			std::scoped_lock lckHooks(rState.mtxHooks);
			bool bAllDone = true;
			if (OMEGAWARE_GAME_DEPTH)
				OmegaRenderHook::Pipeline::PrepareEarly(false);
			for (stExport& rExport : rState.aExports)
			{
				if (rExport.bDone)
					continue;
				const HMODULE hModule = GetModuleHandleW(rExport.szModule);
				if (!hModule)
				{
					bAllDone = false;
					continue;
				}
				rExport.bDone = true;
				if (void* pTarget = reinterpret_cast<void*>(GetProcAddress(hModule, rExport.szName)))
					static_cast<void>(rExport.clHook.Install(pTarget, rExport.pDetour));
			}
			return bAllDone;
		}

		[[nodiscard]] bool HasVisibleWindow()
		{
			bool bFound = false;
			EnumWindows([](HWND hWindow, LPARAM pFound) -> BOOL {
				DWORD uProcessId = 0;
				GetWindowThreadProcessId(hWindow, &uProcessId);
				if (uProcessId != GetCurrentProcessId() || !IsWindowVisible(hWindow) || GetWindow(hWindow, GW_OWNER))
					return TRUE;
				*reinterpret_cast<bool*>(pFound) = true;
				return FALSE;
			}, reinterpret_cast<LPARAM>(&bFound));
			return bFound;
		}

		[[nodiscard]] bool HasGraphicsModule()
		{
			for (const wchar_t* szModule : { L"d3d9.dll", L"d3d11.dll", L"d3d12.dll", L"opengl32.dll", L"vulkan-1.dll" })
				if (GetModuleHandleW(szModule))
					return true;
			return false;
		}
	}

	void Start()
	{
		stState& rState = State();
		if (rState.bPolling)
			return;
		InitializeExports(rState);
		rState.bStopped = false;
		rState.bPolling = true;
		const bool bAllDone = PollExports(rState);
		if (!bAllDone)
			rState.thPoll = std::thread([] {
				stState& rPoll = State();
				while (rPoll.bPolling && !PollExports(rPoll))
					std::this_thread::sleep_for(10ms);
			});
	}

	EResult Wait(std::chrono::milliseconds msTimeout)
	{
		stState& rState = State();
		const std::int64_t iDeadline = NowMs() + msTimeout.count();
		std::int64_t iVisibleSince = 0;
		bool bLoggedFirst = false;
		while (NowMs() < iDeadline)
		{
			if (IsUnloadRequested())
				return EResult::Cancelled;
			const std::int64_t iNow = NowMs();
			const int iPresents = rState.iPresents.load(std::memory_order_acquire);
			if (iPresents > 0 && !bLoggedFirst)
			{
				bLoggedFirst = true;
				LogInfo("the game presented its first {} frame", ApiName());
			}
			if (iPresents >= iReadyPresents && iNow - rState.iFirstPresentMs >= iReadyMs)
				return EResult::Rendering;
			const std::int64_t iVulkanDevice = rState.iVulkanDeviceMs.load(std::memory_order_acquire);
			const bool bVisible = HasVisibleWindow();
			if (iVulkanDevice && iPresents == 0 && bVisible && iNow - iVulkanDevice >= iVulkanSettleMs)
				return EResult::Rendering;
			if (bVisible && HasGraphicsModule())
			{
				if (!iVisibleSince)
					iVisibleSince = iNow;
				const bool bMissedStart = !rState.bSawCreation.load(std::memory_order_acquire);
				if (iPresents == 0 && !iVulkanDevice && iNow - iVisibleSince >= (bMissedStart ? iMissedStartMs : iFallbackMs))
					return EResult::Fallback;
			}
			else
				iVisibleSince = 0;
			std::this_thread::sleep_for(50ms);
		}
		return EResult::TimedOut;
	}

	void Stop()
	{
		stState& rState = State();
		rState.bPolling = false;
		if (rState.thPoll.joinable())
			rState.thPoll.join();
		{
			std::scoped_lock lckHooks(rState.mtxHooks);
			rState.bStopped = true;
			for (stExport& rExport : rState.aExports)
				rExport.clHook.Disable();
			for (stGroup& rGroup : rState.aGroups)
				for (OmegaRenderHook::clInlineHook& rHook : rGroup.aHooks)
					rHook.Disable();
		}
		const auto tpDeadline = std::chrono::steady_clock::now() + 1s;
		while (rState.iCalls.load(std::memory_order_acquire) > 0 && std::chrono::steady_clock::now() < tpDeadline)
			std::this_thread::sleep_for(1ms);
		std::this_thread::sleep_for(20ms);
		if (rState.iCalls.load(std::memory_order_acquire) > 0)
		{
			LogWarning("readiness hooks still had calls in flight, leaving them disabled in place");
			return;
		}
		std::scoped_lock lckHooks(rState.mtxHooks);
		for (stExport& rExport : rState.aExports)
			rExport.clHook.Uninstall();
		for (stGroup& rGroup : rState.aGroups)
		{
			for (OmegaRenderHook::clInlineHook& rHook : rGroup.aHooks)
				rHook.Uninstall();
			rGroup.aTargets = {};
			rGroup.uCount = 0;
		}
	}

	std::string_view ApiName()
	{
		return kApiNames[static_cast<std::size_t>(State().eApi.load())];
	}

	int Presents()
	{
		return State().iPresents.load();
	}
}
