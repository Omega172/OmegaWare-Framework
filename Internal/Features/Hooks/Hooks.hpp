#pragma once
#include "pch.h"

class Hooks : public BaseFeature
{
public:
	bool Setup();

	void Destroy();

	void Run();

	std::string GetName() { return "Hooks"; };
};