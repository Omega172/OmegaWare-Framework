#include "pch.h"
#include <sstream>
#include <fstream>

// These functions might be ugly but they are used to make the console text look pretty and make debugging easier



Utils::Location Utils::GetLocation(std::source_location stLocation)
{
	return { std::filesystem::path(stLocation.file_name()).filename().string(), stLocation.function_name(), stLocation.line(), stLocation.column() };
}

void Utils::LogHook(Location stLocation, std::string sHookName, std::string sReason, std::string sMessage)
{
#ifdef _DEBUG
	// Hook[HookName]: Filename | Function() -> Ln: 1 Col: 1 | Reason: Message
	std::cout << colors::cyan << "Hook[" << sHookName << "]" << colors::white << ": ";

	std::cout << colors::green << stLocation.m_sFilename << colors::white << " | " << colors::green << stLocation.m_sFunction << colors::white;

	std::cout << " -> Ln: " << colors::magenta << stLocation.m_iLine << colors::white << " Col: " << colors::magenta << stLocation.m_iColumn << colors::white;

	std::cout << " | " << colors::yellow << sReason << colors::white << ": " << sMessage << std::endl;
#endif
}

void Utils::LogError(Location stLocation, int iErrorCode)
{
#ifdef _DEBUG
	std::stringstream SS;
	SS << Utils::GetDocumentsFolder() << "\\\\" << Framework::Framework << "\\\\";
	if (!std::filesystem::exists(SS.str()))
		std::filesystem::create_directory(SS.str());

	SS << FRAMEWORK_TARGET_GAME << "_ERRORS.log";

	std::ofstream ErrorFile(SS.str(), std::ios::app);
	if (ErrorFile.fail())
	{
		LogErrorHere("Failed to open ERROR log file for writing");
		return;
	}

	// Error: Filename | Function() -> Ln: 1 Col: 1 | Info: Message
	std::cout << colors::red << "Error" << colors::white << ": ";
	ErrorFile << "Error" << ": ";

	std::cout << colors::green << stLocation.m_sFilename << colors::white << " | " << colors::green << stLocation.m_sFunction << colors::white;
	ErrorFile << stLocation.m_sFilename << " | " << stLocation.m_sFunction;

	std::cout << " -> Ln: " << colors::magenta << stLocation.m_iLine << colors::white << " Col: " << colors::magenta << stLocation.m_iColumn << colors::white;
	ErrorFile << " -> Ln: " << stLocation.m_iLine << " Col: " << stLocation.m_iColumn;

	std::cout << " | " << colors::yellow << "Info" << colors::white << ": " << std::system_category().message(iErrorCode) << std::endl;
	ErrorFile << " | " << "Info" << ": " << std::system_category().message(iErrorCode) << std::endl;

	if (ErrorFile.is_open())
		ErrorFile.close();
#endif
}

void Utils::LogError(Location stLocation, std::string sErrorMessage)
{
#ifdef _DEBUG
	std::stringstream SS;
	SS << Utils::GetDocumentsFolder() << "\\\\" << Framework::Framework << "\\\\";
	if (!std::filesystem::exists(SS.str()))
		std::filesystem::create_directory(SS.str());

	SS << FRAMEWORK_TARGET_GAME << "_ERRORS.log";

	std::ofstream ErrorFile(SS.str(), std::ios::app);
	if (ErrorFile.fail())
	{
		LogErrorHere("Failed to open ERROR log file for writing");
		return;
	}

	// Error: Filename | Function() -> Ln: 1 Col: 1 | Info: Message
	std::cout << colors::red << "Error" << colors::white << ": ";
	ErrorFile << "Error" << ": ";

	std::cout << colors::green << stLocation.m_sFilename << colors::white << " | " << colors::green << stLocation.m_sFunction << colors::white;
	ErrorFile << stLocation.m_sFilename << " | " << stLocation.m_sFunction;

	std::cout << " -> Ln: " << colors::magenta << stLocation.m_iLine << colors::white << " Col: " << colors::magenta << stLocation.m_iColumn << colors::white;
	ErrorFile << " -> Ln: " << stLocation.m_iLine << " Col: " << stLocation.m_iColumn;

	std::cout << " | " << colors::yellow << "Info" << colors::white << ": " << sErrorMessage << std::endl;
	ErrorFile << " | " << "Info" << ": " << sErrorMessage << std::endl;

	if (ErrorFile.is_open())
		ErrorFile.close();
#endif
}

void Utils::LogDebug(Location stLocation, std::string sDebugMessage)
{
#ifdef _DEBUG
	// Debug: Filename | Function() -> Ln: 1 Col: 1 | Info: Message
	std::cout << colors::cyan << "Debug" << colors::white << ": ";

	std::cout << colors::green << stLocation.m_sFilename << colors::white << " | " << colors::green << stLocation.m_sFunction << colors::white;

	std::cout << " -> Ln: " << colors::magenta << stLocation.m_iLine << colors::white << " Col: " << colors::magenta << stLocation.m_iColumn << colors::white;

	std::cout << " | " << colors::yellow << "Info" << colors::white << ": " << sDebugMessage << std::endl;
#endif
}