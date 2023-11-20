#include "pch.h"

Utils::Location Utils::GetLocation(std::source_location stLocation)
{
	return { std::filesystem::path(stLocation.file_name()).filename().string(), stLocation.function_name(), stLocation.line(), stLocation.column() };
}

void Utils::LogError(Location stLocation, int iErrorCode)
{
#ifdef _DEBUG
	// Error: Filename | Function() -> Ln: 1 Col: 1 | Info: Message
	std::cout << colors::red << "Error" << colors::white << ": ";

	std::cout << colors::green << stLocation.m_sFilename << colors::white << " | " << colors::green << stLocation.m_sFunction << colors::white;

	std::cout << " -> Ln: " << colors::magenta << stLocation.m_iLine << colors::white << " Col: " << colors::magenta << stLocation.m_iColumn << colors::white;

	std::cout << " | " << colors::yellow << "Info" << colors::white << ": " << std::system_category().message(iErrorCode) << std::endl;
#endif
}

void Utils::LogError(Location stLocation, std::string sErrorMessage)
{
#ifdef _DEBUG
	// Error: Filename | Function() -> Ln: 1 Col: 1 | Info: Message
	std::cout << colors::red << "Error" << colors::white << ": ";

	std::cout << colors::green << stLocation.m_sFilename << colors::white << " | " << colors::green << stLocation.m_sFunction << colors::white;

	std::cout << " -> Ln: " << colors::magenta << stLocation.m_iLine << colors::white << " Col: " << colors::magenta << stLocation.m_iColumn << colors::white;

	std::cout << " | " << colors::yellow << "Info" << colors::white << ": " << sErrorMessage << std::endl;
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