#pragma once
#include "pch.h"

class ExampleFeature : public Feature
{
private:
	bool Initialized = false;
	std::mutex Mutex;

	bool bExampleFeature = false;

public:
	ExampleFeature();

	bool Setup();

	void Destroy();

	void HandleKeys();

	void PopulateMenu();

	void Render();

	void Run();

	void SaveConfig();

	void LoadConfig();
};