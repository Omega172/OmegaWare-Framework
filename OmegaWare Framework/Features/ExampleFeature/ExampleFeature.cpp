#include "pch.h"

ExampleFeature::ExampleFeature() {};

bool ExampleFeature::Setup()
{
	std::vector<LocaleData> EnglishLocale = {
		{ HASH("EXAMPLE_FEATURE"), "Example Feature" },
		{ HASH("EXAMPLE_FEATURE_HW"), "Hello, World!" }
	};
	if (!Cheat::localization->AddToLocale("ENG", EnglishLocale))
		return false;

	std::vector<LocaleData> GermanLocale = {
		{ HASH("EXAMPLE_FEATURE"), "Beispielfunktion" },
		{ HASH("EXAMPLE_FEATURE_HW"), "Hallo Welt!" }
	};
	if (!Cheat::localization->AddToLocale("GER", GermanLocale))
		return false;

	std::vector<LocaleData> PolishLocale = {
		{ HASH("EXAMPLE_FEATURE"), "Przykładowa Funkcja" },
		{ HASH("EXAMPLE_FEATURE_HW"), "Cześć Świecie!" }
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
		guiSection->AddElement(static_cast<ElementBase*>(guiCheckboxText.get()), "EXAMPLE_FEATURE", {});
		guiSection->AddElement(static_cast<ElementBase*>(guiCheckbox.get()), "EXAMPLE_FEATURE", {});
		guiSection->AddElement(static_cast<ElementBase*>(guiEnabledText.get()), "EXAMPLE_FEATURE_HW", {});
	});

	if (!guiSection->HasParent())
	{
		Cheat::menu->AddElement(static_cast<ElementBase*>(guiSection.get()), "EXAMPLE_FEATURE_SECTION", {
			.iFlags = ImGuiChildFlags_Border,
		});
	}

	guiEnabledText->SetVisible(guiCheckbox->GetValue());
}