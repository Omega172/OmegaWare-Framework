#include "pch.h"

ExampleFeature::ExampleFeature() {};

bool ExampleFeature::Setup()
{
	Localization::AddToLocale("ENG", std::initializer_list<std::pair<size_t, std::string>>{
		{ "EXAMPLE_FEATURE"_hash, "Example Feature" },
		{ "EXAMPLE_FEATURE_HW"_hash, "Hello, World!" },
		{ "EXAMPLE_FEATURE_SLIDER"_hash, "Cool slider boi?" },
		{ "EXAMPLE_COLORPICKER"_hash, "Color pickah!" },
	});

	Localization::AddToLocale("GER", std::initializer_list<std::pair<size_t, std::string>>{
		{ "EXAMPLE_FEATURE"_hash, "Beispielfunktion" },
		{ "EXAMPLE_FEATURE_HW"_hash, "Hallo Welt!" }
	});

	Localization::AddToLocale("POL", std::initializer_list<std::pair<size_t, std::string>>{
		{ "EXAMPLE_FEATURE"_hash, "Przykładowa Funkcja" },
		{ "EXAMPLE_FEATURE_HW"_hash, "Cześć Świecie!" }
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