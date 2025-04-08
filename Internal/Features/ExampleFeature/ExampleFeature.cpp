#include "pch.h"
#include "ExampleFeature.hpp"

bool ExampleFeature::SetupMenu()
{
	Localization::AddToLocale("ENG", std::initializer_list<std::pair<size_t, std::string>>{
		{ "EXAMPLE_FEATURE"Hashed, "Example Feature" },
		{ "EXAMPLE_FEATURE_HW"Hashed, "Hello, World!" },
		{ "EXAMPLE_FEATURE_SLIDER"Hashed, "Example Slider" },
		{ "EXAMPLE_COLORPICKER"Hashed, "Example Color Picker" }
	});

	Localization::AddToLocale("GER", std::initializer_list<std::pair<size_t, std::string>>{
		{ "EXAMPLE_FEATURE"Hashed, "Beispiel funktion" },
		{ "EXAMPLE_FEATURE_HW"Hashed, "Hallo Welt!" },
		{ "EXAMPLE_FEATURE_SLIDER"Hashed, "Beispiel Slider" },
		{ "EXAMPLE_COLORPICKER"Hashed, "Beispiel Farbwähler" }
	});

	Localization::AddToLocale("POL", std::initializer_list<std::pair<size_t, std::string>>{
		{ "EXAMPLE_FEATURE"Hashed, "Przykładowa Funkcja" },
		{ "EXAMPLE_FEATURE_HW"Hashed, "Cześć Świecie!" },
		{ "EXAMPLE_FEATURE_SLIDER"Hashed, "Przykładowy suwak" },
		{ "EXAMPLE_COLORPICKER"Hashed, "Przykładowy próbnik kolorów" }
	});

	return true;
}

void ExampleFeature::HandleMenu()
{
	if (!m_pBodyGroup->HasParent())
		Framework::menu->GetChild("HEADER_GROUP")->GetChild("BODY")->AddElement(m_pBodyGroup.get());
}