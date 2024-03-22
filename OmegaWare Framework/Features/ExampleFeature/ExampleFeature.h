#pragma once
#include "pch.h"

class ExampleFeature : public Feature
{
private:
	bool Initialized = false;
	std::mutex Mutex;

	std::unique_ptr<Child> guiSection = std::make_unique<Child>([]() { return ImVec2(ImGui::GetContentRegionAvail().x / 2, ImGui::GetContentRegionAvail().y / 2); });
	std::unique_ptr<Text> guiCheckboxText = std::make_unique<Text>();
	std::unique_ptr<Checkbox> guiCheckbox = std::make_unique<Checkbox>();
	std::unique_ptr<Text> guiEnabledText = std::make_unique<Text>();

public:
	ExampleFeature();

	bool Setup();

	void Destroy();

	void HandleKeys();

	void Render();

	void Run();

	void HandleMenu();
};