#pragma once
#include "pch.h"

class ExampleFeature : public Feature
{
private:
	bool Initialized = false;
	std::mutex Mutex;

	std::unique_ptr<Child> GuiSection = std::make_unique<Child>("EXAMPLE_FEATURE", "EXAMPLE_FEATURE"Hashed, ElementBase::Style_t{
		.iFlags = ImGuiChildFlags_Border | ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY }, ImGuiWindowFlags_HorizontalScrollbar);
	std::unique_ptr<Checkbox> GuiCheckbox = std::make_unique<Checkbox>("CHECKBOX", "EXAMPLE_FEATURE"Hashed);
	std::unique_ptr<Text> GuiEnabledText = std::make_unique<Text>("CHECKBOX_ENABLED_TEXT", "EXAMPLE_FEATURE_HW"Hashed);
	std::unique_ptr<SliderInt> GuiEnabledSlider = std::make_unique<SliderInt>("CHECKBOX_ENABLED_SLIDER", "EXAMPLE_FEATURE_SLIDER"Hashed);
	std::unique_ptr<Text> GuiColorPickerLabel = std::make_unique<Text>("EXAMPLE_COLORPICKER_LABEL", "EXAMPLE_COLORPICKER"Hashed);
	std::unique_ptr<ColorPicker> GuiColorPicker = std::make_unique<ColorPicker>("EXAMPLE_COLORPICKER", "EXAMPLE_COLORPICKER"Hashed, ElementBase::Style_t{
		.iFlags = ImGuiColorEditFlags_NoInputs});

public:
	ExampleFeature();

	bool Setup();

	void Destroy();

	void HandleKeys();

	void Render();

	void Run();

	void HandleMenu();
};