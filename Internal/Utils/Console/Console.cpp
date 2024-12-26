#include "pch.h"

Console::Console(bool bVisibility, std::string sConsoleTitle, DWORD dwMode)
{
	if (m_bInitalized)
		return;

	if (!AllocConsole())
	{		
		MessageBoxA(NULL, std::format("Failed to allocate console! Error: {}", GetLastError()).c_str(), "Error", MB_ICONERROR);
		Utils::LogError(std::to_string(GetLastError())); // This code is here only to log the error, if another console is already attached
		return;
	}

	errno_t errSTDOut = freopen_s(&m_pSTDOutDummy, "CONOUT$", "w", stdout);
	if (errSTDOut != NULL)
	{
		Utils::LogError(std::to_string(errSTDOut));
		return;
	}

	errno_t errSTDErr = freopen_s(&m_pSTDErrDummy, "CONOUT$", "w", stderr);
	if (errSTDErr != NULL)
	{
		Utils::LogError(std::to_string(errSTDErr));
		return;
	}

	errno_t errSTDIn = freopen_s(&m_pSTDInDummy, "CONIN$", "w", stdin);
	if (errSTDIn != NULL)
	{
		Utils::LogError(std::to_string(errSTDIn));
		return;
	}

	std::cout.clear();
	std::cerr.clear();
	std::cin.clear();
	
	SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), dwMode);

	SetTitle(sConsoleTitle);
	SetVisibility(bVisibility);

	m_bInitalized = true;
	return;
}

void Console::Destroy()
{
	if (!m_bInitalized)
		return;

	if (m_pSTDOutDummy)
		fclose(m_pSTDOutDummy);

	if (m_pSTDInDummy)
		fclose(m_pSTDInDummy);

	if (!FreeConsole())
		Utils::LogError(std::to_string(GetLastError()));

	m_bInitalized = false;
	return;
}

void Console::SetVisibility(bool bVisibility)
{
	m_bVisible = bVisibility;
	ShowWindow(GetConsoleWindow(), (bVisibility) ? SW_SHOW : SW_HIDE);
	return;
}

void Console::ToggleVisibility()
{
	SetVisibility(!m_bVisible);
	return;
}

void Console::SetMode(DWORD dwMode)
{
	SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), dwMode);
	return;
}
