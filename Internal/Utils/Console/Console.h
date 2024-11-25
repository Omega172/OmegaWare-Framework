// I made this forever ago. It's a simple console class that can be used to create a console window and hide it. It's useful for debugging and testing

#pragma once
#include <iostream>
#include <format>

class Console
{
private:
	FILE* m_pSTDOutDummy = nullptr;
	FILE* m_pSTDInDummy = nullptr;

	bool m_bAllocated  : 1 = false;
	bool m_bInitalized : 1 = false;
	bool m_bVisible    : 1 = false;

public:
	enum Visibility
	{
		SHOWN = true,
		HIDDEN = false
	};

	Console(bool bVisibility, std::string sConsoleTitle = std::format("DEBUG CONSOLE | x64"));
	void Destroy();

	void SetTitle(std::string sTitle) { SetConsoleTitleA(sTitle.c_str()); }

	bool GetVisibility() { return this->m_bVisible; };
	void SetVisibility(bool bVisibility);
	void ToggleVisibility();
};