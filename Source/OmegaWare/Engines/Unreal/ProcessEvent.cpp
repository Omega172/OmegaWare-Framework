#include "Engines/Unreal/ProcessEvent.hpp"
#include "Engines/Unreal/Resolver.hpp"

#if OMEGAWARE_ENGINE_UNREAL

#include "Core/Log.hpp"
#include "Hooks/Hook.hpp"
#include "Memory/Safety.hpp"

#include "OmegaRenderHook/Core/Guarded.hpp"
#include "OmegaRenderHook/Hook/HookScope.hpp"
#include "OmegaRenderHook/Hook/InlineHook.hpp"

#include <Windows.h>
#include <TlHelp32.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>
#include <vector>
#include <thread>

namespace OmegaWare::Unreal
{
	namespace
	{
		using FnProcessEventTarget = void (*)(const SDK::UObject*, SDK::UFunction*, void*);
		using clEngineScope = OmegaRenderHook::clHookScopeOf<OmegaRenderHook::EHookDomain::Engine>;

		struct stEntry
		{
			HookHandle uHandle = kInvalidHook;
			std::string strFunction;
			FnProcessEvent fnBefore;
			FnProcessEvent fnAfter;
			SDK::UFunction* pFunction = nullptr;
			std::atomic<bool> bFaulted = false;
			std::atomic<std::uint64_t> uCalls = 0;
		};

		using EntryList = std::vector<std::shared_ptr<stEntry>>;

		std::shared_mutex g_mtxEntries;
		std::unordered_map<SDK::UFunction*, EntryList> g_mapResolved;
		EntryList g_aPending;
		std::atomic<std::size_t> g_uResolved = 0;
		std::atomic<HookHandle> g_uNextHandle = 1;
		std::mutex g_mtxInstall;
		OmegaRenderHook::clInlineHook g_clProcessEvent;
		bool g_bInstalled = false;
		auto g_tpNextRetry = std::chrono::steady_clock::time_point{};

		struct stTask
		{
			std::function<void()> fnTask;
			std::shared_ptr<std::atomic<bool>> pDone;
		};

		std::mutex g_mtxTasks;
		std::vector<stTask> g_aTasks;
		std::atomic<bool> g_bTasksPending = false;
		std::atomic<bool> g_bAcceptTasks = true;
		std::atomic<DWORD> g_uGameThread = 0;
		std::atomic<bool> g_bLoggedGameThread = false;
		thread_local bool t_bRunningTasks = false;

		[[nodiscard]] DWORD FindMainThread()
		{
			const HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
			if (hSnapshot == INVALID_HANDLE_VALUE)
				return 0;
			const DWORD uProcess = GetCurrentProcessId();
			DWORD uOldest = 0;
			ULARGE_INTEGER uOldestTime{ .QuadPart = ~0ull };
			THREADENTRY32 stEntry{ .dwSize = sizeof(THREADENTRY32) };
			for (BOOL bMore = Thread32First(hSnapshot, &stEntry); bMore; bMore = Thread32Next(hSnapshot, &stEntry))
			{
				if (stEntry.th32OwnerProcessID != uProcess)
					continue;
				const HANDLE hThread = OpenThread(THREAD_QUERY_LIMITED_INFORMATION, FALSE, stEntry.th32ThreadID);
				if (!hThread)
					continue;
				FILETIME stCreated{}, stExited{}, stKernel{}, stUser{};
				if (GetThreadTimes(hThread, &stCreated, &stExited, &stKernel, &stUser))
				{
					const ULARGE_INTEGER uCreated{ .LowPart = stCreated.dwLowDateTime, .HighPart = stCreated.dwHighDateTime };
					if (uCreated.QuadPart < uOldestTime.QuadPart)
					{
						uOldestTime = uCreated;
						uOldest = stEntry.th32ThreadID;
					}
				}
				CloseHandle(hThread);
			}
			CloseHandle(hSnapshot);
			return uOldest;
		}

		[[nodiscard]] DWORD GameThreadId()
		{
			DWORD uThread = g_uGameThread.load(std::memory_order_acquire);
			if (!uThread)
			{
				uThread = FindMainThread();
				g_uGameThread.store(uThread, std::memory_order_release);
				LogInfo("unreal: game thread is the process main thread ({})", uThread);
			}
			return uThread;
		}

		void RunTasks()
		{
			std::vector<stTask> aTasks;
			{
				std::scoped_lock lckTasks(g_mtxTasks);
				aTasks.swap(g_aTasks);
				g_bTasksPending.store(false, std::memory_order_release);
			}
			t_bRunningTasks = true;
			for (stTask& rTask : aTasks)
			{
				Hooks::clCallGuard clGuard;
				OmegaRenderHook::stFault stFault;
				auto fnInvoke = [&] { rTask.fnTask(); };
				if (!OmegaRenderHook::InvokeGuarded(fnInvoke, stFault))
				{
					if (stFault.bCppException)
						LogError("game thread task threw a C++ exception");
					else
						LogError("game thread task faulted (code 0x{:08X} at {})", stFault.uCode, stFault.pAddress);
				}
				if (rTask.pDone)
					rTask.pDone->store(true, std::memory_order_release);
			}
			t_bRunningTasks = false;
		}

		[[nodiscard]] std::string FullFunctionName(std::string_view strFunction)
		{
			return strFunction.starts_with("Function ") ? std::string(strFunction) : "Function " + std::string(strFunction);
		}

		[[nodiscard]] SDK::UFunction* FindFunction(const std::string& strFullName)
		{
			return SDK::UObject::FindObject<SDK::UFunction>(strFullName, SDK::EClassCastFlags::Function);
		}

		void RunCallback(stEntry& rEntry, const FnProcessEvent& fnCallback, stProcessEventCall& rCall)
		{
			if (!fnCallback || rEntry.bFaulted.load(std::memory_order_relaxed))
				return;
			Hooks::clCallGuard clGuard;
			auto fnInvoke = [&] { fnCallback(rCall); };
			OmegaRenderHook::stFault stFault;
			if (OmegaRenderHook::InvokeGuarded(fnInvoke, stFault))
				return;
			rEntry.bFaulted = true;
			if (stFault.bCppException)
				LogError("process event hook on {} threw a C++ exception and was disabled", rEntry.strFunction);
			else
				LogError("process event hook on {} faulted (code 0x{:08X} at {}) and was disabled", rEntry.strFunction, stFault.uCode, stFault.pAddress);
		}

		void CallOriginal(const SDK::UObject* pObject, SDK::UFunction* pFunction, void* pParams)
		{
			g_clProcessEvent.Original<FnProcessEventTarget>()(pObject, pFunction, pParams);
		}

		void DetourProcessEvent(const SDK::UObject* pObject, SDK::UFunction* pFunction, void* pParams)
		{
			clEngineScope clScope;
			if (g_bTasksPending.load(std::memory_order_acquire) && !t_bRunningTasks && GetCurrentThreadId() == g_uGameThread.load(std::memory_order_relaxed))
			{
				if (!g_bLoggedGameThread.exchange(true))
					LogInfo("unreal: game thread tasks running on thread {}", GetCurrentThreadId());
				RunTasks();
			}
			if (!pFunction || g_uResolved.load(std::memory_order_acquire) == 0)
				return CallOriginal(pObject, pFunction, pParams);

			EntryList aEntries;
			{
				std::shared_lock lckEntries(g_mtxEntries);
				const auto itEntries = g_mapResolved.find(pFunction);
				if (itEntries != g_mapResolved.end())
					aEntries = itEntries->second;
			}
			if (aEntries.empty())
				return CallOriginal(pObject, pFunction, pParams);

			stProcessEventCall stCall{ const_cast<SDK::UObject*>(pObject), pFunction, pParams, false };
			for (const auto& pEntry : aEntries)
			{
				pEntry->uCalls.fetch_add(1, std::memory_order_relaxed);
				RunCallback(*pEntry, pEntry->fnBefore, stCall);
			}
			if (!stCall.bSkipOriginal)
				CallOriginal(pObject, pFunction, pParams);
			for (const auto& pEntry : aEntries)
				RunCallback(*pEntry, pEntry->fnAfter, stCall);
		}

		[[nodiscard]] bool EnsureInstalled()
		{
			std::scoped_lock lckInstall(g_mtxInstall);
			if (g_bInstalled)
				return true;
			void* pTarget = Resolver::ProcessEventAddress();
			if (!pTarget)
				return false;
			if (!Memory::IsExecutable(pTarget) || !g_clProcessEvent.Install(pTarget, reinterpret_cast<void*>(&DetourProcessEvent)))
				return false;
			g_bInstalled = true;
			LogInfo("UObject::ProcessEvent hooked at {}", pTarget);
			return true;
		}

		void Resolve(const std::shared_ptr<stEntry>& pEntry, SDK::UFunction* pFunction)
		{
			pEntry->pFunction = pFunction;
			g_mapResolved[pFunction].push_back(pEntry);
			g_uResolved.fetch_add(1, std::memory_order_acq_rel);
		}
	}

	HookHandle OnProcessEvent(std::string_view strFunction, FnProcessEvent fnBefore, FnProcessEvent fnAfter)
	{
		if (!fnBefore && !fnAfter)
			return kInvalidHook;
		if (!EnsureInstalled())
		{
			LogError("could not hook UObject::ProcessEvent, {} is not hooked", strFunction);
			return kInvalidHook;
		}
		auto pEntry = std::make_shared<stEntry>();
		pEntry->uHandle = g_uNextHandle.fetch_add(1, std::memory_order_relaxed);
		pEntry->strFunction = FullFunctionName(strFunction);
		pEntry->fnBefore = std::move(fnBefore);
		pEntry->fnAfter = std::move(fnAfter);
		SDK::UFunction* pFunction = FindFunction(pEntry->strFunction);

		std::unique_lock lckEntries(g_mtxEntries);
		if (pFunction)
			Resolve(pEntry, pFunction);
		else
		{
			g_aPending.push_back(pEntry);
			LogInfo("{} is not loaded yet, its process event hook waits for it", pEntry->strFunction);
		}
		return pEntry->uHandle;
	}

	bool RemoveProcessEvent(HookHandle uHandle)
	{
		std::unique_lock lckEntries(g_mtxEntries);
		const auto fnMatch = [uHandle](const std::shared_ptr<stEntry>& pEntry) { return pEntry->uHandle == uHandle; };
		if (std::erase_if(g_aPending, fnMatch))
			return true;
		for (auto itEntries = g_mapResolved.begin(); itEntries != g_mapResolved.end(); ++itEntries)
		{
			if (!std::erase_if(itEntries->second, fnMatch))
				continue;
			if (itEntries->second.empty())
				g_mapResolved.erase(itEntries);
			g_uResolved.fetch_sub(1, std::memory_order_acq_rel);
			return true;
		}
		return false;
	}

	void RetryPendingProcessEvents()
	{
		const auto tpNow = std::chrono::steady_clock::now();
		EntryList aPending;
		{
			std::shared_lock lckEntries(g_mtxEntries);
			if (g_aPending.empty() || tpNow < g_tpNextRetry)
				return;
			aPending = g_aPending;
		}
		g_tpNextRetry = tpNow + std::chrono::seconds(2);
		for (const auto& pEntry : aPending)
		{
			SDK::UFunction* pFunction = FindFunction(pEntry->strFunction);
			if (!pFunction)
				continue;
			std::unique_lock lckEntries(g_mtxEntries);
			if (!std::erase(g_aPending, pEntry))
				continue;
			Resolve(pEntry, pFunction);
			LogInfo("{} loaded, its process event hook is active", pEntry->strFunction);
		}
	}

	bool IsGameThread()
	{
		return GetCurrentThreadId() == GameThreadId();
	}

	bool RunOnGameThread(std::function<void()> fnTask)
	{
		if (!fnTask || !g_bAcceptTasks.load(std::memory_order_acquire) || !EnsureInstalled() || !GameThreadId())
			return false;
		std::scoped_lock lckTasks(g_mtxTasks);
		g_aTasks.push_back({ std::move(fnTask), nullptr });
		g_bTasksPending.store(true, std::memory_order_release);
		return true;
	}

	bool RunOnGameThreadAndWait(std::function<void()> fnTask, std::chrono::milliseconds msTimeout)
	{
		if (!fnTask)
			return false;
		if (IsGameThread())
		{
			fnTask();
			return true;
		}
		if (!g_bAcceptTasks.load(std::memory_order_acquire) || !EnsureInstalled() || !GameThreadId())
			return false;
		auto pDone = std::make_shared<std::atomic<bool>>(false);
		{
			std::scoped_lock lckTasks(g_mtxTasks);
			g_aTasks.push_back({ std::move(fnTask), pDone });
			g_bTasksPending.store(true, std::memory_order_release);
		}
		const auto tpDeadline = std::chrono::steady_clock::now() + msTimeout;
		while (!pDone->load(std::memory_order_acquire))
		{
			if (std::chrono::steady_clock::now() >= tpDeadline)
			{
				LogWarning("game thread task did not run within {} ms", msTimeout.count());
				return false;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
		return true;
	}

	void ShutdownProcessEvent()
	{
		g_bAcceptTasks.store(false, std::memory_order_release);
		{
			std::scoped_lock lckTasks(g_mtxTasks);
			g_aTasks.clear();
			g_bTasksPending.store(false, std::memory_order_release);
		}
		{
			std::unique_lock lckEntries(g_mtxEntries);
			g_mapResolved.clear();
			g_aPending.clear();
			g_uResolved = 0;
		}
		std::scoped_lock lckInstall(g_mtxInstall);
		if (!g_bInstalled)
			return;
		const bool bDisabled = g_clProcessEvent.Disable();
		if (!bDisabled || !clEngineScope::WaitForDrain(std::chrono::seconds(2)))
		{
			LogWarning("UObject::ProcessEvent calls were still in flight, leaving the disabled hook in place");
			return;
		}
		g_clProcessEvent.Uninstall();
		g_bInstalled = false;
		LogInfo("UObject::ProcessEvent unhooked");
	}

	std::vector<stProcessEventHookInfo> ProcessEventHooks()
	{
		std::shared_lock lckEntries(g_mtxEntries);
		std::vector<stProcessEventHookInfo> aHooks;
		const auto fnAdd = [&](const stEntry& rEntry, bool bResolved) { aHooks.push_back({ rEntry.uHandle, rEntry.strFunction, bResolved, rEntry.bFaulted, rEntry.uCalls }); };
		for (const auto& [pFunction, aEntries] : g_mapResolved)
			for (const auto& pEntry : aEntries)
				fnAdd(*pEntry, true);
		for (const auto& pEntry : g_aPending)
			fnAdd(*pEntry, false);
		std::ranges::sort(aHooks, {}, &stProcessEventHookInfo::uHandle);
		return aHooks;
	}
}

#endif
