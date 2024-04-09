#include "pch.h"

ExampleFeature::ExampleFeature() {};

bool ExampleFeature::Setup()
{
	Localization::AddToLocale("ENG", std::initializer_list<std::pair<size_t, std::string>>{
		{ HashString("EXAMPLE_FEATURE"), "Example Feature" },
		{ HashString("EXAMPLE_FEATURE_HW"), "Hello, World!" },
		{ HashString("EXAMPLE_FEATURE_SLIDER"), "Cool slider boi?" },
		{ HashString("EXAMPLE_COLORPICKER"), "Color pickah!" },
	});

	Localization::AddToLocale("GER", std::initializer_list<std::pair<size_t, std::string>>{
		{ HashString("EXAMPLE_FEATURE"), "Beispielfunktion" },
		{ HashString("EXAMPLE_FEATURE_HW"), "Hallo Welt!" }
	});

	Localization::AddToLocale("POL", std::initializer_list<std::pair<size_t, std::string>>{
		{ HashString("EXAMPLE_FEATURE"), "Przykładowa Funkcja" },
		{ HashString("EXAMPLE_FEATURE_HW"), "Cześć Świecie!" }
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
		guiSection->SetCallback([]() {
			ImGuiContext* pContext = ImGui::GetCurrentContext();

			ImVec2 vec2Size = (Framework::menu->m_stStyle.vec2Size / ImVec2{ 3.f, 2.f }) - pContext->Style.ItemSpacing;
			ImVec2 vec2MaxSize = ImGui::GetContentRegionAvail();

			if (vec2Size.x > vec2MaxSize.x)
				vec2Size.x = vec2MaxSize.x;

			if (vec2Size.y > vec2MaxSize.y)
				vec2Size.y = vec2MaxSize.y;

			return vec2Size;
		});

		guiSection->AddElement(guiCheckboxText.get());
		guiSection->AddElement(guiCheckbox.get());

		guiCheckbox->AddElement(guiEnabledText.get());
		guiCheckbox->AddElement(guiEnabledSlider.get());

		guiSectionDos->SetCallback([]() {
			ImGuiContext* pContext = ImGui::GetCurrentContext();

			ImVec2 vec2Size = (Framework::menu->m_stStyle.vec2Size / ImVec2{ 3.f, 2.f }) - pContext->Style.ItemSpacing;
			ImVec2 vec2MaxSize = ImGui::GetContentRegionAvail();

			if (vec2Size.x > vec2MaxSize.x)
				vec2Size.x = vec2MaxSize.x;

			if (vec2Size.y > vec2MaxSize.y)
				vec2Size.y = vec2MaxSize.y;

			return vec2Size;
		});
		guiSectionDos->AddElement(guiColorPicker.get());
	});

	if (!guiSection->HasParent())
		Framework::menu->AddElement(guiSection.get());

	if (!guiSectionDos->HasParent())
		Framework::menu->AddElement(guiSectionDos.get());

	guiCheckbox->SetChildrenVisible(guiCheckbox->GetValue());
}