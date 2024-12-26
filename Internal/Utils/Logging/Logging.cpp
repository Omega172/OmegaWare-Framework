#include "pch.h"
#include <sstream>
#include <fstream>
#include <format>
#include "Colors.hpp" // Include the colors header file which contains the color class used to set the color of the console text to make it look pretty
#include <filesystem>

// I hate this.
static std::string ConvertFunctionName(const char* szFunctionName)
{
	std::string sFnName{ szFunctionName };

	size_t iEnd = sFnName.find_last_of('(');
	if (iEnd == std::string::npos) {
		iEnd = sFnName.size();
	}

	int iTemplates = 0;

	bool bDone = false;

	if (iEnd != sFnName.size()) {
		for (size_t i = iEnd - 1; i > 0 && !bDone; --i) {
			const char c = sFnName[i];

			switch (c) {
			case('<'):
				iTemplates--;
				break;
			case('>'):
				iTemplates++;
				break;

			default:
				if (iTemplates == 0) {
					bDone = true;
					iEnd = i;
				}
				break;
			}
		}
	}

	iTemplates = 0;
	bool bHasValidChar = false;
	size_t iStart = 0;

	for (size_t i = iEnd; i > 0 && iStart == 0; --i) {
		const char c = sFnName[i];
		switch (c) {
		case('<'):
			bHasValidChar = false;
			iTemplates--;
			break;
		case('>'):
			bHasValidChar = false;
			iTemplates++;
			break;
		case(' '):
			if (bHasValidChar) {
				iStart = i + 1;
			}
			break;

		case(';'):
		case(':'):
		case('('):
		case(')'):
		case('{'):
		case('}'):
			bHasValidChar = false;
			break;

		default:
			if (iTemplates == 0) {
				bHasValidChar = true;
			}
			break;
		}
	}

	return std::string(sFnName, iStart, iEnd - iStart + 1);
}

static std::string GetLocationString(std::source_location location) 
{
	return std::format("[{}{}{} ({}{}{},{}{}{})] {}{}{} |",
		colors::green, std::filesystem::path(location.file_name()).filename().string(), colors::white,
		colors::magenta, location.line(), colors::white, 
		colors::magenta, location.column(), colors::white,
		colors::green, ConvertFunctionName(location.function_name()), colors::white);
}

static std::string GetColorlessLocationString(std::source_location location) 
{
	return std::format("{} | {} -> Ln: {} Col: {} |", location.file_name(), ConvertFunctionName(location.function_name()), location.line(), location.column());
}

static void LogError(const std::string& sErrorMessage, std::source_location location) {
	// Error: [$Filename ($Ln,$Col)] $Function | Info: $Message
	std::cout << std::format("{}Error{}: {} {}Info{}: {}\n", colors::red, colors::white, GetLocationString(location), colors::yellow, colors::white, sErrorMessage);
}

static void LogDebug(const std::string& sDebugMessage, std::source_location location) {
	// Debug: [$Filename ($Ln,$Col)] $Function | Info: $Message
	std::cout << std::format("{}Debug{}: {} {}Info{}: {}\n", colors::cyan, colors::white, GetLocationString(location), colors::yellow, colors::white, sDebugMessage);
}

namespace Utils {
	void LogHook(const std::string& sHookName, const MH_STATUS eStatus, std::source_location location)
	{
	#ifdef _DEBUG
		if (eStatus == MH_OK) {
			// Hook[$HookName]: [$Filename ($Ln,$Col)] $Function | Success!
			std::cout << std::format("{}Hook[{}]{}: {} {}Success{}!\n", colors::cyan, sHookName, colors::white, GetLocationString(location), colors::green, colors::white);
			return;
		}

		// Hook[$HookName]: [$Filename ($Ln,$Col)] $Function | Failure: $Status
		std::cout << std::format("{}Hook[{}]{}: {} {}Failure{}: {}\n", colors::cyan, sHookName, colors::white, GetLocationString(location), colors::yellow, colors::white, MH_StatusToString(eStatus));
	#endif
	}
	void LogHook(const std::string& sHookName, const std::string& sReason, const std::string& sMessage, std::source_location location)
	{
	#ifdef _DEBUG
		// Hook[$HookName]: [$Filename ($Ln,$Col)] $Function | $Reason: $Message
		std::cout << std::format("{}Hook[{}]{}: {} {}{}{}: {}\n", colors::cyan, sHookName, colors::white, GetLocationString(location), colors::yellow, sReason, colors::white, sMessage);
	#endif
	}

	void LogError(const int iErrorCode, std::source_location location) 
	{
	#ifdef _DEBUG
		::LogError(std::system_category().message(iErrorCode), location);

		std::filesystem::path pathError{};
		{
			auto optPath = Utils::GetLogFilePath("_ERRORS.log");
			if (!optPath) {
				::LogError("Failed to find ERROR log file path", std::source_location::current());
				return;
			}

			pathError = optPath.value();
		}

		std::ofstream fileError(pathError, std::ios::app);
		if (!fileError.is_open() || fileError.fail())
		{
			::LogError("Failed to open ERROR log file for writing", std::source_location::current());
			return;
		}

		fileError << std::format("Error: {} Info: {}\n", GetColorlessLocationString(location), std::system_category().message(iErrorCode));
		fileError.close();
	#endif
	}

	void LogError(const std::string& sErrorMessage, std::source_location location)
	{
	#ifdef _DEBUG
		::LogError(sErrorMessage, location);

		std::filesystem::path pathError{};
		{
			auto optPath = Utils::GetLogFilePath("_ERRORS.log");
			if (!optPath) {
				::LogError("Failed to find ERROR log file path", std::source_location::current());
				return;
			}

			pathError = optPath.value();
		}

		std::ofstream fileError(pathError, std::ios::app);
		if (!fileError.is_open() || fileError.fail())
		{
			::LogError("Failed to open ERROR log file for writing", std::source_location::current());
			return;
		}

		fileError << std::format("Error: {} Info: {}\n", GetColorlessLocationString(location), sErrorMessage);
		fileError.close();
	#endif
	}

	void LogDebug(const std::string& sDebugMessage, std::source_location location)
	{
	#ifdef _DEBUG
		::LogDebug(sDebugMessage, location);

		std::filesystem::path pathDebug{};
		{
			auto optPath = Utils::GetLogFilePath(".log");
			if (!optPath) {
				::LogError("Failed to find DEBUG log file path", std::source_location::current());
				return;
			}

			pathDebug = optPath.value();
		}

		std::ofstream fileDebug(pathDebug, std::ios::app);
		if (!fileDebug.is_open() || fileDebug.fail())
		{
			::LogError("Failed to open DEBUG log file for writing", std::source_location::current());
			return;
		}

		fileDebug << std::format("Debug: {} Info: {}\n", GetColorlessLocationString(location), sDebugMessage);
		fileDebug.close();
	#endif
	}
};