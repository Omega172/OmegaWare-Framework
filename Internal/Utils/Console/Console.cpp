#include "pch.h"

Console::Console(bool bVisibility, std::string sConsoleTitle)
{
	if (m_bInitalized)
		return;

	if (!AllocConsole())
	{		
		MessageBoxA(NULL, std::format("Failed to allocate console! Error: {}", GetLastError()).c_str(), "Error", MB_ICONERROR);
		LogErrorHere(std::to_string(GetLastError())); // This code is here only to log the error, if another console is already attached
		m_bInitalized = false;
		return;
	}

	errno_t errSTDOut = freopen_s(&m_pSTDOutDummy, "CONOUT$", "w", stdout);
	if (errSTDOut != NULL)
	{
		LogErrorHere(std::to_string(errSTDOut));
		m_bInitalized = false;
		return;
	}

	errno_t errSTDIn = freopen_s(&m_pSTDInDummy, "CONIN$", "w", stdin);
	if (errSTDIn != NULL)
	{
		LogErrorHere(std::to_string(errSTDIn));
		m_bInitalized = false;
		return;
	}

	std::cout.clear();
	std::cin.clear();
	
	SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING);

	SetVisibility(bVisibility);
	SetTitle(sConsoleTitle);

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
		LogErrorHere(std::to_string(GetLastError()));

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