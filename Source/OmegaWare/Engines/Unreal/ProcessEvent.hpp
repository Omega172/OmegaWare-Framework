#pragma once

#if OMEGAWARE_ENGINE_UNREAL

#include "Engines/Unreal/Unreal.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace OmegaWare::Unreal
{
	struct stProcessEventCall
	{
		SDK::UObject* pObject = nullptr;
		SDK::UFunction* pFunction = nullptr;
		void* pParams = nullptr;
		bool bSkipOriginal = false;
	};

	using FnProcessEvent = std::function<void(stProcessEventCall&)>;
	using HookHandle = std::uint32_t;
	inline constexpr HookHandle kInvalidHook = 0;

	struct stProcessEventHookInfo
	{
		HookHandle uHandle = kInvalidHook;
		std::string strFunction;
		bool bResolved = false;
		bool bFaulted = false;
		std::uint64_t uCalls = 0;
	};

	HookHandle OnProcessEvent(std::string_view strFunction, FnProcessEvent fnBefore, FnProcessEvent fnAfter = {});
	bool RemoveProcessEvent(HookHandle uHandle);
	void RetryPendingProcessEvents();
	void ShutdownProcessEvent();

	[[nodiscard]] bool IsGameThread();
	bool RunOnGameThread(std::function<void()> fnTask);
	bool RunOnGameThreadAndWait(std::function<void()> fnTask, std::chrono::milliseconds msTimeout);
	[[nodiscard]] std::vector<stProcessEventHookInfo> ProcessEventHooks();

	template <typename Params>
	HookHandle OnFunction(std::string_view strFunction, std::function<void(SDK::UObject*, Params&, bool&)> fnBefore, std::function<void(SDK::UObject*, Params&)> fnAfter = {})
	{
		FnProcessEvent fnRawBefore;
		if (fnBefore)
			fnRawBefore = [fnBefore = std::move(fnBefore)](stProcessEventCall& rCall) { fnBefore(rCall.pObject, *static_cast<Params*>(rCall.pParams), rCall.bSkipOriginal); };
		FnProcessEvent fnRawAfter;
		if (fnAfter)
			fnRawAfter = [fnAfter = std::move(fnAfter)](stProcessEventCall& rCall) { fnAfter(rCall.pObject, *static_cast<Params*>(rCall.pParams)); };
		return OnProcessEvent(strFunction, std::move(fnRawBefore), std::move(fnRawAfter));
	}
}

#endif
