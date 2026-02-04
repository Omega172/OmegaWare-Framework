#include "pch.h"
#include "ExampleFeature.hpp"

bool ExampleFeature::SetupMenu()
{
	Localization::AddToLocale("ENG", std::initializer_list<std::pair<size_t, std::string>>{
		{ "EXAMPLE_FEATURE"Hashed, "Example Feature" },
		{ "EXAMPLE_FEATURE_HW"Hashed, "Hello, World!" },
		{ "EXAMPLE_FEATURE_SLIDER"Hashed, "Example Slider" },
		{ "EXAMPLE_COLORPICKER"Hashed, "Example Color Picker" },

		{ "EXAMPLE_FEATURE_BUTTON"Hashed, "Example Feature"},
		{ "EXAMPLE_MAIN"Hashed, "Main" },
		{ "EXAMPLE_SECONDARY"Hashed, "Example" },

		{ "EXAMPLE_MAIN_PAGE_GROUP_CHILD"Hashed, "Group" },
		{ "EXAMPLE_CHECKBOX"Hashed, "Log Values To Console" },
		{ "EXAMPLE_COLORPICKER"Hashed, "##Color Picker" },
		{ "EXAMPLE_MAIN_PAGE_GROUP_CHILD2"Hashed, "Group 2" },
		{ "EXAMPLE_TEXT"Hashed, "This is some text" },
		{ "EXAMPLE_TOGGLE"Hashed, "Toggle" },
		
		{ "EXAMPLE_MAIN_PAGE_CHILD"Hashed, "Group 3" },
		{ "EXAMPLE_COMBO"Hashed, "Combo" },
		{ "EXAMPLE_INPUTTEXT"Hashed, "Input Text" },
		{ "EXAMPLE_SLIDERINT"Hashed, "Slider Int" },
		{ "EXAMPLE_SLIDERFLOAT"Hashed, "Slider Float" },
		{ "EXAMPLE_ACCENTBUTTON"Hashed, "Accent Button" },

		{ "EXAMPLE_SECONDARY_CHILD"Hashed, "Subpage 2" },
		{ "EXAMPLE_SECONDARY_TEXT"Hashed, "This is some text on the secondary page." }
	});

	Localization::AddToLocale("GER", std::initializer_list<std::pair<size_t, std::string>>{
		{ "EXAMPLE_FEATURE"Hashed, "Beispiel Funktion" },
		{ "EXAMPLE_FEATURE_HW"Hashed, "Hallo Welt!" },
		{ "EXAMPLE_FEATURE_SLIDER"Hashed, "Beispiel-Schieberegler" },
		{ "EXAMPLE_COLORPICKER"Hashed, "Beispiel Farbwähler" },

		{ "EXAMPLE_FEATURE_BUTTON"Hashed, "Beispiel Funktion"},
		{ "EXAMPLE_MAIN"Hashed, "Haupt" },
		{ "EXAMPLE_SECONDARY"Hashed, "Sekundär" },

		{ "EXAMPLE_MAIN_PAGE_GROUP_CHILD"Hashed, "Gruppe" },
		{ "EXAMPLE_CHECKBOX"Hashed, "Werte in Konsole protokollieren" },
		{ "EXAMPLE_MAIN_PAGE_GROUP_CHILD2"Hashed, "Gruppe 2" },
		{ "EXAMPLE_TEXT"Hashed, "Dies ist ein Text" },
		{ "EXAMPLE_TOGGLE"Hashed, "Umschalten" },
		
		{ "EXAMPLE_MAIN_PAGE_CHILD"Hashed, "Gruppe 3" },
		{ "EXAMPLE_COMBO"Hashed, "Auswahl" },
		{ "EXAMPLE_INPUTTEXT"Hashed, "Texteingabe" },
		{ "EXAMPLE_SLIDERINT"Hashed, "Ganzzahl-Schieberegler" },
		{ "EXAMPLE_SLIDERFLOAT"Hashed, "Dezimal-Schieberegler" },
		{ "EXAMPLE_ACCENTBUTTON"Hashed, "Akzent-Schaltfläche" },

		{ "EXAMPLE_SECONDARY_CHILD"Hashed, "Unterseite 2" },
		{ "EXAMPLE_SECONDARY_TEXT"Hashed, "Dies ist ein Text auf der Sekundärseite." }
	});

	Localization::AddToLocale("POL", std::initializer_list<std::pair<size_t, std::string>>{
		{ "EXAMPLE_FEATURE"Hashed, "Przykładowa Funkcja" },
		{ "EXAMPLE_FEATURE_HW"Hashed, "Cześć Świecie!" },
		{ "EXAMPLE_FEATURE_SLIDER"Hashed, "Przykładowy suwak" },
		{ "EXAMPLE_COLORPICKER"Hashed, "Przykładowy próbnik kolorów" },

		{ "EXAMPLE_FEATURE_BUTTON"Hashed, "Przykładowa Funkcja"},
		{ "EXAMPLE_MAIN"Hashed, "Główna" },
		{ "EXAMPLE_SECONDARY"Hashed, "Przykład" },

		{ "EXAMPLE_MAIN_PAGE_GROUP_CHILD"Hashed, "Grupa" },
		{ "EXAMPLE_CHECKBOX"Hashed, "Rejestruj wartości w konsoli" },
		{ "EXAMPLE_MAIN_PAGE_GROUP_CHILD2"Hashed, "Grupa 2" },
		{ "EXAMPLE_TEXT"Hashed, "To jest jakiś tekst" },
		{ "EXAMPLE_TOGGLE"Hashed, "Przełącznik" },
		
		{ "EXAMPLE_MAIN_PAGE_CHILD"Hashed, "Grupa 3" },
		{ "EXAMPLE_COMBO"Hashed, "Wybór" },
		{ "EXAMPLE_INPUTTEXT"Hashed, "Wprowadzanie tekstu" },
		{ "EXAMPLE_SLIDERINT"Hashed, "Suwak liczb całkowitych" },
		{ "EXAMPLE_SLIDERFLOAT"Hashed, "Suwak liczb zmiennoprzecinkowych" },
		{ "EXAMPLE_ACCENTBUTTON"Hashed, "Przycisk akcentu" },

		{ "EXAMPLE_SECONDARY_CHILD"Hashed, "Podstrona 2" },
		{ "EXAMPLE_SECONDARY_TEXT"Hashed, "To jest jakiś tekst na stronie drugorzędnej." }
	});

	return true;
}

void ExampleFeature::HandleMenu()
{
	static std::once_flag onceflag;
	std::call_once(onceflag, [this]() {
		Framework::menu->GetChild("SIDEBAR")->InsertElementAfter(m_pExampleFeatureButton.get(), "FEATURE_SEPERATOR");
		
		auto pHeaderGroup = static_cast<HeaderGroup*>(Framework::menu->GetChild("HEADER_GROUP"));
		if (pHeaderGroup)
			pHeaderGroup->AddHeaders(ExampleFeature::s_iExamplePageId, { "EXAMPLE_MAIN"Hashed, "EXAMPLE_SECONDARY"Hashed });

		m_pMainPageGroupChild->SetCallback([]() {
			float fGroupWidth = (ImGui::GetWindowWidth() - 10.0f - 10.0f * 2) / 2;
			float fGroupHeight = (ImGui::GetWindowHeight() - 10.0f - 10.0f * 2) / 2;
			return ImVec2(fGroupWidth, fGroupHeight);
		});

		m_pMainPageGroupChild->AddElement(m_pExampleCheckbox.get());

		m_pExampleColorPicker->SetSameLineSizeCallback([]() {
			return ImVec2(ImGui::GetWindowWidth() - 10.0f - ImGui::GetFontSize() * 2, 0);
		});

		m_pExampleColorPicker->SetValue(ImVec4(0.0f, 1.0f, 1.0f, 1.0f));

		m_pMainPageGroupChild->AddElement(m_pExampleColorPicker.get());
		m_pMainPageGroup->AddElement(m_pMainPageGroupChild.get());

		m_pMainPageGroupChild2->SetCallback([]() {
			float fGroupWidth = (ImGui::GetWindowWidth() - 10.0f - 10.0f * 2) / 2;
			return ImVec2(fGroupWidth, 0);
		});

		m_pMainPageGroupChild2->AddElement(m_pExampleText.get());
		m_pMainPageGroupChild2->AddElement(m_pExampleToggle.get());
		m_pMainPageGroup->AddElement(m_pMainPageGroupChild2.get());

		m_pExampleCombo->AddOption("Option 1", [this]() {
			if (!m_pExampleCheckbox->GetValue())
				return;

			Utils::LogDebug("ExampleFeature: Selected Option 1");
		});
		m_pExampleCombo->AddOption("Option 2", [this]() {
			if (!m_pExampleCheckbox->GetValue())
				return;

			Utils::LogDebug("ExampleFeature: Selected Option 2");
		});
		m_pExampleCombo->AddOption("Option 3", [this]() {
			if (!m_pExampleCheckbox->GetValue())
				return;

			Utils::LogDebug("ExampleFeature: Selected Option 3");
		});

		m_pMainPageChild->AddElement(m_pExampleCombo.get());
		m_pMainPageChild->AddElement(m_pExampleInputText.get());
		m_pMainPageChild->AddElement(m_pExampleSliderInt.get());
		m_pMainPageChild->AddElement(m_pExampleSliderFloat.get());
		m_pMainPageChild->AddElement(m_pExampleSeperator.get());

		m_pExampleAccentButton->SetCallback([this]() {
			if (!m_pExampleCheckbox->GetValue())
				return;

			Utils::LogDebug("ExampleFeature: Accent Button Pressed");
		});
		m_pMainPageChild->AddElement(m_pExampleAccentButton.get());

		m_pMainPage->AddElement(m_pMainPageGroup.get());
		m_pMainPage->AddElement(m_pMainPageChild.get());

		m_pSecondaryPageChild->AddElement(m_pSecondaryPageText.get());
		m_pSecondaryPage->AddElement(m_pSecondaryPageChild.get());

		Framework::menu->GetChild("HEADER_GROUP")->GetChild("BODY")->AddElement(m_pMainPage.get());
		Framework::menu->GetChild("HEADER_GROUP")->GetChild("BODY")->AddElement(m_pSecondaryPage.get());
	});
}

void ExampleFeature::Run()
{
	if (m_pExampleCheckbox->GetValue())
	{
		Utils::LogDebug(std::format("ExampleFeature: Color Picker Value: R:{} G:{} B:{} A:{}", 
			(int)(m_pExampleColorPicker->GetValue().x * 255), 
			(int)(m_pExampleColorPicker->GetValue().y * 255), 
			(int)(m_pExampleColorPicker->GetValue().z * 255), 
			(int)(m_pExampleColorPicker->GetValue().w * 255)));

		Utils::LogDebug(std::format("ExampleFeature: Slider Int Value: {}", m_pExampleSliderInt->GetValue()));
		Utils::LogDebug(std::format("ExampleFeature: Slider Float Value: {}", m_pExampleSliderFloat->GetValue()));
	}
}