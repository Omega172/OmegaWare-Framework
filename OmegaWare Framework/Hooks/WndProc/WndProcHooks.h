#pragma once

#include "pch.h"

class WndProcHooks
{
private:

public:
	HWND hwndWindow;
	DWORD dwProcessId;

	bool Setup();

	void Destroy();
};