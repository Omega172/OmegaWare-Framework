#include "pch.h"

ExampleFeature::ExampleFeature() {};

bool ExampleFeature::Setup()
{
	std::vector<LocaleData> EnglishLocale = {
		{ HASH("EXAMPLE_FEATURE"), "Example Feature" },
		{ HASH("EXAMPLE_FEATURE_HW"), "Hello, World!" }
	};
	if (!Framework::localization->AddToLocale("ENG", EnglishLocale))
		return false;

	std::vector<LocaleData> GermanLocale = {
		{ HASH("EXAMPLE_FEATURE"), "Beispielfunktion" },
		{ HASH("EXAMPLE_FEATURE_HW"), "Hallo Welt!" }
	};
	if (!Framework::localization->AddToLocale("GER", GermanLocale))
		return false;

	std::vector<LocaleData> PolishLocale = {
		{ HASH("EXAMPLE_FEATURE"), "Przykładowa Funkcja" },
		{ HASH("EXAMPLE_FEATURE_HW"), "Cześć Świecie!" }
	};
	if (!Framework::localization->AddToLocale("POL", PolishLocale))
		return false;

	Framework::localization->UpdateLocale();

	LogDebugHere("Feature: ExampleFeature Initialized");

	Initialized = true;
	return Initialized;
}

void ExampleFeature::Destroy() { Initialized = false; }

void ExampleFeature::HandleKeys() {}

void ExampleFeature::PopulateMenu()
{
	if (!Initialized)
		return;

	auto ExampleFeature = std::make_unique<Child>("ExampleFeature", []() { return ImVec2(ImGui::GetContentRegionAvail().x / 2, ImGui::GetContentRegionAvail().y / 2); }, ImGuiChildFlags_Border);
	ExampleFeature->AddElement(std::make_unique<Text>(Framework::localization->Get("EXAMPLE_FEATURE")));
	ExampleFeature->AddElement(std::make_unique<Checkbox>(Framework::localization->Get("EXAMPLE_FEATURE"), &bExampleFeature));
	if (bExampleFeature)
		ExampleFeature->AddElement(std::make_unique<Text>(Framework::localization->Get("EXAMPLE_FEATURE_HW")));

	Framework::menu->AddElement(std::move(ExampleFeature), true);
}

void ExampleFeature::Render() {}

void ExampleFeature::Run() {}

void ExampleFeature::SaveConfig() { Framework::config->PushEntry("ExampleFeature", "bool", std::to_string(bExampleFeature)); }

void ExampleFeature::LoadConfig()
{
	ConfigEntry entry = Framework::config->GetEntryByName("ExampleFeature");
	if (entry.Name == "ExampleFeature")
		bExampleFeature = std::stoi(entry.Value);
}