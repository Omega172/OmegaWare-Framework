#pragma once
#include "pch.h"

class ExampleFeature : public Feature
{
private:
	bool Initalized = false;
	bool bExampleFeature = false;

public:
	ExampleFeature();

	bool Setup();

	void Destroy();

	void HandleKeys();

	void DrawMenuItems();

	void Render();

	void Run();

	void SaveConfig();

	void LoadConfig();
};