#pragma once

namespace Hooking
{
	enum class HookStatus : uint8_t
	{
		Uninstalled,
		Installed,
		Failed,
	};

	inline const char* ToString(HookStatus eStatus)
	{
		switch (eStatus)
		{
		case HookStatus::Uninstalled: return "Uninstalled";
		case HookStatus::Installed:   return "Installed";
		case HookStatus::Failed:      return "Failed";
		default:                      return "Unknown";
		}
	}
}
