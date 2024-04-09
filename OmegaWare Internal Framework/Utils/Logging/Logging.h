#pragma once
#include "Colors.hpp" // Include the colors header file which contains the color class used to set the color of the console text to make it look pretty

#include <source_location>
#include <filesystem>

#define CurrentLoc (std::source_location::current())

#define LogHookHere(hook, reason, message) (Utils::LogHook(Utils::GetLocation(CurrentLoc), hook, reason, message))
#define LogErrorHere(message) (Utils::LogError(Utils::GetLocation(CurrentLoc), message))
#define LogErrorStreamHere(message) (Utils::LogError(Utils::GetLocation(CurrentLoc), (std::stringstream() << message).str()))
#define LogDebugHere(message) (Utils::LogDebug(Utils::GetLocation(CurrentLoc), message))
#define LogDebugStreamHere(message) (Utils::LogDebug(Utils::GetLocation(CurrentLoc), (std::stringstream() << message).str()))


namespace Utils
{
	struct Location // A struct to hold the location of the error used in the logging function for pretty printing
	{
		std::string m_sFilename;
		std::string m_sFunction;
		unsigned int m_iLine;
		unsigned int m_iColumn;
	};

	Location GetLocation(std::source_location stLocation); // Convert the source location to a location struct

	// Various logging functions for different types of messages
	void LogHook(Location stLocation, std::string sHookName, std::string sReason, std::string sMessage);

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