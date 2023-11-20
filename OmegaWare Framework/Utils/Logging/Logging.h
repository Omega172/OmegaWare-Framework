#pragma once
#include "Colors.hpp"

#include <source_location>
#include <filesystem>

namespace Utils
{
	struct Location
	{
		std::string m_sFilename;
		std::string m_sFunction;
		unsigned int m_iLine;
		unsigned int m_iColumn;
	};

	Location GetLocation(std::source_location stLocation);

	void LogError(Location stLocation, int iErrorCode);
	void LogError(Location stLocation, std::string sErrorMessage);

	void LogDebug(Location stLocation, std::string sDebugMessage);

	// Because std::format verifies values at compile time sometimes we need to use its runtime version std::vforamt this is my wrapper for it
	template<typename... Args>
	inline std::string _Format(const std::format_string<Args...> fmt, Args&&... args)
	{
		return std::vformat(fmt.get(), std::make_format_args(args...));
	}
}