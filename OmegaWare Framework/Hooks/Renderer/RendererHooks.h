#pragma once
#include "pch.h"

class RendererHooks
{
private:
	bool Initialized = false;

public:

	bool Setup();
	void Destroy();
};