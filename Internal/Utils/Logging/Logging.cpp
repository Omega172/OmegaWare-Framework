#include "pch.h"
#include <sstream>
#include <fstream>
#include <format>

// These functions might be ugly but they are used to make the console text look pretty and make debugging easier



Utils::Location Utils::GetLocation(std::source_location stLocation)
{
	return { std::filesystem::path(stLocation.file_name()).filename().string(), stLocation.function_name(), stLocation.line(), stLocation.column() };
}

void Utils::LogHook(Location stLocation, std::string sHookName, std::string sReason, std::string sMessage)
{
#ifdef _DEBUG
	// Hook[HookName]: Filename | Function() -> Ln: 1 Col: 1 | Reason: Message
	std::cout << std::format("{}Hook[{}]{}: {}{}{} | {}{}{} -> Ln: {}{}{} Col: {}{}{} | {}{}{}: {}\n",
		colors::cyan, sHookName, colors::white,
		colors::green, stLocation.m_sFilename, colors::white, colors::green, stLocation.m_sFunction, colors::white,
		colors::magenta, stLocation.m_iLine, colors::white, colors::magenta, stLocation.m_iColumn, colors::white,
		colors::yellow, sReason, colors::white, sMessage);
#endif
}

void Utils::LogError(Location stLocation, int iErrorCode)
{
#ifdef _DEBUG
	std::filesystem::path pathError{};
	{
		auto optPath = Utils::GetLogFilePath("_ERRORS.log");
		if (!optPath) {
			LogErrorHere("Failed to open ERROR log folder for writing");
			return;
		}

		pathError = optPath.value();
	}

	std::ofstream fileError(pathError, std::ios::app);
	if (!fileError.is_open() || fileError.fail())
	{
		LogErrorHere("Failed to open ERROR log file for writing");
		return;
	}

	// Error: Filename | Function() -> Ln: 1 Col: 1 | Info: Message
	std::cout << std::format("{}Error{}: {}{}{} | {}{}{} -> Ln: {}{}{} Col: {}{}{} | {}Info{}: {}\n",
		colors::red, colors::white,
		colors::green, stLocation.m_sFilename, colors::white, colors::green, stLocation.m_sFunction, colors::white,
		colors::magenta, stLocation.m_iLine, colors::white, colors::magenta, stLocation.m_iColumn, colors::white,
		colors::yellow, colors::white, std::system_category().message(iErrorCode));

	fileError << std::format("Error: {} | {} -> Ln: {} Col: {} | Info: {}\n",
		stLocation.m_sFilename, stLocation.m_sFunction, stLocation.m_iLine, stLocation.m_iColumn, std::system_category().message(iErrorCode));

	fileError.close();
#endif
}

void Utils::LogError(Location stLocation, std::string sErrorMessage)
{
#ifdef _DEBUG
	std::filesystem::path pathError{};
	{
		auto optPath = Utils::GetLogFilePath("_ERRORS.log");
		if (!optPath) {
			LogErrorHere("Failed to open ERROR log folder for writing");
			return;
		}

		pathError = optPath.value();
	}

	std::ofstream fileError(pathError, std::ios::app);
	if (!fileError.is_open() || fileError.fail())
	{
		LogErrorHere("Failed to open ERROR log file for writing");
		return;
	}

	// Error: Filename | Function() -> Ln: 1 Col: 1 | Info: Message
	std::cout << std::format("{}Error{}: {}{}{} | {}{}{} -> Ln: {}{}{} Col: {}{}{} | {}Info{}: {}\n",
		colors::red, colors::white,
		colors::green, stLocation.m_sFilename, colors::white, colors::green, stLocation.m_sFunction, colors::white,
		colors::magenta, stLocation.m_iLine, colors::white, colors::magenta, stLocation.m_iColumn, colors::white,
		colors::yellow, colors::white, sErrorMessage);

	fileError << std::format("Error: {} | {} -> Ln: {} Col: {} | Info: {}\n",
		stLocation.m_sFilename, stLocation.m_sFunction, stLocation.m_iLine, stLocation.m_iColumn, sErrorMessage);

	fileError.close();
#endif
}

void Utils::LogDebug(Location stLocation, std::string sDebugMessage)
{
#ifdef _DEBUG
	// Debug: Filename | Function() -> Ln: 1 Col: 1 | Info: Message
	std::cout << std::format("{}Debug{}: {}{}{} | {}{}{} -> Ln: {}{}{} Col: {}{}{} | {}Info{}{}",
		colors::cyan, colors::white,
		colors::green, stLocation.m_sFilename, colors::white, colors::green, stLocation.m_sFunction, colors::white,
		colors::magenta, stLocation.m_iLine, colors::white, colors::magenta, stLocation.m_iColumn, colors::white,
		colors::yellow, colors::white, sDebugMessage);
#endif
}