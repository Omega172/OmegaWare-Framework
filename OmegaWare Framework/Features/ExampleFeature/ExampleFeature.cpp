#include "pch.h"

ExampleFeature::ExampleFeature() {};

bool ExampleFeature::Setup()
{
	if (!Cheat::localization->AddToLocale("ENG", "EXAMPLE_FEATURE", "Example Feature"))
		return false;

	if (!Cheat::localization->AddToLocale("ENG", "EXAMPLE_FEATURE_HW", "Hello, World!"))
		return false;

	if (!Cheat::localization->SetLocale("ENG"))
	{
		Utils::LogError(Utils::GetLocation(CurrentLoc), "Failed to update locale ENG");
		return false;
	}

	Initalized = true;
	return Initalized;
}

void ExampleFeature::Destroy() { Initalized = false; }

void ExampleFeature::HandleKeys() {}

void ExampleFeature::DrawMenuItems()
{
	if (!Initalized)
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