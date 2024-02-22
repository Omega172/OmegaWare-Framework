#pragma once

#include "pch.h"

class WndProcHooks
{
private:

public:
	HWND hwndWindow;

	bool Setup();

	void Destroy();
};