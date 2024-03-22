#pragma once
#include "pch.h"

class ExampleFeature : public Feature
{
private:
	bool Initialized = false;
	std::mutex Mutex;

	std::unique_ptr<Child> guiSection = std::make_unique<Child>("EXAMPLE_FEATURE", "EXAMPLE_FEATURE", ElementBase::Style_t{
		.iFlags = ImGuiChildFlags_Border,
	});
	std::unique_ptr<Text> guiCheckboxText = std::make_unique<Text>("CHECKBOX_TEXT", "EXAMPLE_FEATURE"_hash);
	std::unique_ptr<Checkbox> guiCheckbox = std::make_unique<Checkbox>("CHECKBOX", "EXAMPLE_FEATURE"_hash);
	std::unique_ptr<Text> guiEnabledText = std::make_unique<Text>("CHECKBOX_ENABLED_TEXT", "EXAMPLE_FEATURE_HW"_hash);

public:
	ExampleFeature();

	bool Setup();

	void Destroy();

	void HandleKeys();

	void Render();

	void Run();

	void HandleMenu();
};