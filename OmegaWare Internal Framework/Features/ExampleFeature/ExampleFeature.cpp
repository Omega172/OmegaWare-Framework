#include "pch.h"

ExampleFeature::ExampleFeature() {};

bool ExampleFeature::Setup()
{
	Localization::AddToLocale("ENG", std::initializer_list<std::pair<size_t, std::string>>{
		{ "EXAMPLE_FEATURE"Hashed, "Example Feature" },
		{ "EXAMPLE_FEATURE_HW"Hashed, "Hello, World!" },
		{ "EXAMPLE_FEATURE_SLIDER"Hashed, "Ex Slider" },
		{ "EXAMPLE_COLORPICKER"Hashed, "Example Color Picker" },
	});

	Localization::AddToLocale("GER", std::initializer_list<std::pair<size_t, std::string>>{
		{ "EXAMPLE_FEATURE"Hashed, "Beispielfunktion" },
		{ "EXAMPLE_FEATURE_HW"Hashed, "Hallo Welt!" }
	});

	Localization::AddToLocale("POL", std::initializer_list<std::pair<size_t, std::string>>{
		{ "EXAMPLE_FEATURE"Hashed, "Przykładowa Funkcja" },
		{ "EXAMPLE_FEATURE_HW"Hashed, "Cześć Świecie!" }
	});

	LogDebugHere("Feature: ExampleFeature Initialized");

	Initialized = true;
	return Initialized;
}

void ExampleFeature::Destroy() { Initialized = false; }

void ExampleFeature::HandleKeys() {}

void ExampleFeature::Render() {}

void ExampleFeature::Run() {}

void ExampleFeature::HandleMenu()
{
	static std::once_flag onceflag;
	std::call_once(onceflag, [this]() {
		GuiSection->SetCallback([]() {
			ImGuiContext* pContext = ImGui::GetCurrentContext();

			ImVec2 vec2Size = (Framework::menu->m_stStyle.vec2Size / ImVec2{ 3.f, 2.f }) - pContext->Style.ItemSpacing;
			ImVec2 vec2MaxSize = ImGui::GetContentRegionAvail();

			if (vec2Size.x > vec2MaxSize.x)
				vec2Size.x = vec2MaxSize.x;

			if (vec2Size.y > vec2MaxSize.y)
				vec2Size.y = vec2MaxSize.y;

			return vec2Size;
		});

		GuiSection->AddElement(GuiCheckbox.get());

		GuiCheckbox->AddElement(GuiEnabledText.get());
		GuiCheckbox->AddElement(GuiEnabledSlider.get());

		GuiSection->AddElement(GuiColorPickerLabel.get());
		GuiSection->AddElement(GuiColorPicker.get());
	});

	if (!GuiSection->HasParent())
		Framework::menu->AddElement(GuiSection.get());

	GuiCheckbox->SetChildrenVisible(GuiCheckbox->GetValue());
}