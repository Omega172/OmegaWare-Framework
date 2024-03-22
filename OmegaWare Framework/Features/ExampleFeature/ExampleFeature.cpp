#include "pch.h"

ExampleFeature::ExampleFeature() {};

bool ExampleFeature::Setup()
{
	std::vector<LocaleData> EnglishLocale = {
		{ "EXAMPLE_FEATURE"_hash, "Example Feature" },
		{ "EXAMPLE_FEATURE_HW"_hash, "Hello, World!" }
	};
	if (!Cheat::localization->AddToLocale("ENG", EnglishLocale))
		return false;

	std::vector<LocaleData> GermanLocale = {
		{ "EXAMPLE_FEATURE"_hash, "Beispielfunktion" },
		{ "EXAMPLE_FEATURE_HW"_hash, "Hallo Welt!" }
	};
	if (!Cheat::localization->AddToLocale("GER", GermanLocale))
		return false;

	std::vector<LocaleData> PolishLocale = {
		{ "EXAMPLE_FEATURE"_hash, "Przykładowa Funkcja" },
		{ "EXAMPLE_FEATURE_HW"_hash, "Cześć Świecie!" }
	};
	if (!Cheat::localization->AddToLocale("POL", PolishLocale))
		return false;

	Cheat::localization->UpdateLocale();

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
		guiSection->SetCallback([]() { return ImVec2(ImGui::GetContentRegionAvail().x / 2, ImGui::GetContentRegionAvail().y / 2); });

		guiSection->AddElement(guiCheckboxText.get());
		guiSection->AddElement(guiCheckbox.get());
		guiSection->AddElement(guiEnabledText.get());
	});

	if (!guiSection->HasParent())
	{
		Cheat::menu->AddElement(guiSection.get());
	}

	guiEnabledText->SetVisible(guiCheckbox->GetValue());
}