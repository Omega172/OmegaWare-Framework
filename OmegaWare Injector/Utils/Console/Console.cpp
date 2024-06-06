#include "Console.h"
#include "../Logging/Logging.h"

Console::Console(bool bVisibility, std::string sConsoleTitle)
{
	if (this->bInitalized)
		return;

	if (!AllocConsole())
	{
		std::stringstream SS("Failed to allocate console! Error: ");
		SS << GetLastError();

		MessageBoxA(NULL, SS.str().c_str(), "Error", MB_ICONERROR);
		LogErrorHere(GetLastError()); // This code is here only to log the error, if another console is already attached
		this->bInitalized = false;
		return;
	}

	errno_t errSTDOut = freopen_s(&m_pSTDOutDummy, "CONOUT$", "w", stdout);
	if (errSTDOut != NULL)
	{
		LogErrorHere(errSTDOut);
		this->bInitalized = false;
		return;
	}

	errno_t errSTDIn = freopen_s(&m_pSTDInDummy, "CONIN$", "w", stdin);
	if (errSTDIn != NULL)
	{
		LogErrorHere(errSTDIn);
		this->bInitalized = false;
		return;
	}

	std::cout.clear();
	std::cin.clear();

	SetVisibility(bVisibility);
	SetTitle(sConsoleTitle);

	this->bInitalized = true;
	return;
}

void Console::Destroy()
{
	if (!this->bInitalized)
		return;

	if (m_pSTDOutDummy)
		fclose(m_pSTDOutDummy);

	if (m_pSTDInDummy)
		fclose(m_pSTDInDummy);

	if (!FreeConsole())
		LogErrorHere(GetLastError());

	this->bInitalized = false;
	return;
}

void Console::SetVisibility(bool bVisibility)
{
	this->bVisible = bVisibility;
	ShowWindow(GetConsoleWindow(), (bVisibility) ? SW_SHOW : SW_HIDE);
	return;
}

void Console::ToggleVisibility()
{
	SetVisibility(!this->bVisible);
	return;
}