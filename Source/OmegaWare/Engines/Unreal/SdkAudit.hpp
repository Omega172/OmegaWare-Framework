#pragma once

#if OMEGAWARE_ENGINE_UNREAL

#include <cstddef>
#include <filesystem>

namespace OmegaWare::Unreal::SdkAudit
{
	struct stSummary
	{
		bool bRan = false;
		std::size_t uOwners = 0;
		std::size_t uMembers = 0;
		std::size_t uMatched = 0;
		std::size_t uMoved = 0;
		std::size_t uNotReflected = 0;
		std::size_t uOwnerMissing = 0;
		std::size_t uOwnersMissing = 0;
		long long llMilliseconds = 0;
		std::filesystem::path pathReport;
	};

	stSummary Run();
}

#endif
