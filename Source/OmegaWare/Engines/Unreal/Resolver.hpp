#pragma once

#if OMEGAWARE_ENGINE_UNREAL

#include <cstdint>

namespace SDK
{
	class UWorld;
}

namespace OmegaWare::Unreal::Resolver
{
	bool Resolve();
	[[nodiscard]] bool IsResolved();
	[[nodiscard]] SDK::UWorld* World();
	[[nodiscard]] void* ProcessEventAddress();
	void LogReport();
	bool LogScanTest();
}

#endif
