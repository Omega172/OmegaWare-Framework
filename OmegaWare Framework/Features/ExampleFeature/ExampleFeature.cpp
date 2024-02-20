#include "pch.h"

ExampleFeature::ExampleFeature() {};

bool ExampleFeature::Setup()
{
	std::vector<LocaleData> EnglishLocale = {
		{ HASH("EXAMPLE_FEATURE"), "Example Feature" },
		{ HASH("EXAMPLE_FEATURE_HW"), "Hello, World!" }
	};
	Cheat::localization->AddToLocale("ENG", EnglishLocale);

	std::vector<LocaleData> GermanLocale = {
		{ HASH("EXAMPLE_FEATURE"), "Beispielfunktion" },
		{ HASH("EXAMPLE_FEATURE_HW"), "Hallo Welt!" }
	};
	Cheat::localization->AddToLocale("GER", GermanLocale);

	Cheat::localization->UpdateLocale();

	Initialized = true;
	return Initialized;
}

void ExampleFeature::Destroy() { Initialized = false; }

void ExampleFeature::HandleKeys() {}

void ExampleFeature::DrawMenuItems()
{
	if (!Initialized)
		return;

	ImGui::SameLine();

	ImGui::BeginChild("ExampleFeature", ImVec2(ImGui::GetContentRegionAvail().x / 2, ImGui::GetContentRegionAvail().y / 2), ImGuiChildFlags_Border);
	{
		ImGui::Text(Cheat::localization->Get("EXAMPLE_FEATURE").c_str());

		ImGui::Checkbox(Cheat::localization->Get("EXAMPLE_FEATURE").c_str(), &bExampleFeature);
		if (bExampleFeature)
			ImGui::Text(Cheat::localization->Get("EXAMPLE_FEATURE_HW").c_str());
	}
	ImGui::EndChild();
}

void ExampleFeature::Render() {}

void ExampleFeature::Run() {}

void ExampleFeature::SaveConfig() { Cheat::config->PushEntry("ExampleFeature", "bool", std::to_string(bExampleFeature)); }

void ExampleFeature::LoadConfig()
{
	ConfigEntry entry = Cheat::config->GetEntryByName("ExampleFeature");
	if (entry.Name == "ExampleFeature")
		bExampleFeature = std::stoi(entry.Value);
}