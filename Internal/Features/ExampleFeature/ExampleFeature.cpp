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
		{ "EXAMPLE_SECONDARY"Hashed, "Example" }
	});

	Localization::AddToLocale("GER", std::initializer_list<std::pair<size_t, std::string>>{
		{ "EXAMPLE_FEATURE"Hashed, "Beispiel Funktion" },
		{ "EXAMPLE_FEATURE_HW"Hashed, "Hallo Welt!" },
		{ "EXAMPLE_FEATURE_SLIDER"Hashed, "Beispiel-Schieberegler" },
		{ "EXAMPLE_COLORPICKER"Hashed, "Beispiel Farbwähler" },

		{ "EXAMPLE_FEATURE_BUTTON"Hashed, "Beispiel Funktion"},
		{ "EXAMPLE_MAIN"Hashed, "Haupt" },
		{ "EXAMPLE_SECONDARY"Hashed, "Sekundär" }
	});

	Localization::AddToLocale("POL", std::initializer_list<std::pair<size_t, std::string>>{
		{ "EXAMPLE_FEATURE"Hashed, "Przykładowa Funkcja" },
		{ "EXAMPLE_FEATURE_HW"Hashed, "Cześć Świecie!" },
		{ "EXAMPLE_FEATURE_SLIDER"Hashed, "Przykładowy suwak" },
		{ "EXAMPLE_COLORPICKER"Hashed, "Przykładowy próbnik kolorów" },

		{ "EXAMPLE_FEATURE_BUTTON"Hashed, "Przykładowa Funkcja"},
		{ "EXAMPLE_MAIN"Hashed, "Główna" },
		{ "EXAMPLE_SECONDARY"Hashed, "Przykład" }
	});

	m_pBodyGroup->SetCallback([this]() {
		float fGroupWidth = (ImGui::GetWindowWidth() - 10.0f - 10.0f * 2) / 2;
		float fGroupHeight = (ImGui::GetWindowHeight() - 10.0f - 10.0f * 2) / 2;

		ImGui::BeginGroup();
		{
			ImGui::BeginChild("group1", ImVec2(fGroupWidth, fGroupHeight), ImGuiChildFlags_Border);
			{
				ImGui::TextDisabled("GROUP 1");

				static bool bEnabled = true;
				static bool bDisabled = false;
				ImAdd::SmallCheckbox("Enabled", &bEnabled);
				ImAdd::SmallCheckbox("Disabled", &bDisabled);

				ImGui::SameLine(ImGui::GetWindowWidth() - 10.0f - ImGui::GetFontSize() * 2);
				static ImVec4 v4Color = ImVec4(1, 1, 0, 0.5f);
				ImAdd::ColorEdit4("##Color Picker", (float*)&v4Color);
			}
			ImGui::EndChild();
			ImGui::BeginChild("group2", ImVec2(fGroupWidth, 0), ImGuiChildFlags_Border);
			{
				ImGui::TextDisabled("GROUP 2");

				static bool bEnabled = true;
				static bool bDisabled = false;
				ImAdd::Togglebutton("Enabled", &bEnabled);
				ImAdd::Togglebutton("Disabled", &bDisabled);
			}
			ImGui::EndChild();
		}
		ImGui::EndGroup();
		ImGui::SameLine();
		ImGui::BeginChild("group3", ImVec2(0, 0), ImGuiChildFlags_Border);
		{
			ImGui::TextDisabled("GROUP 3");
	   
			static int iCombo = 0;
			ImAdd::Combo("Combo", &iCombo, "Item 1\0Item 2\0Item 3\0", -0.1f);
	   
			static char inputText[32] = "";
			ImAdd::InputText("Text Input", "Write something here...", inputText, IM_ARRAYSIZE(inputText), -0.1f);
	   
			static int iSlider = 8;
			static float fSlider = 3;
			ImAdd::SliderInt("Slider Int", &iSlider, 0, 10, -0.1f);
			ImAdd::SliderFloat("Slider Float", &fSlider, 0, 10, -0.1f);
	   
			ImGui::Separator();
	   
			ImAdd::Button("Button", ImVec2(-0.1f, 0));
			ImAdd::AcentButton("Acent button", ImVec2(-0.1f, 0));
		}
		ImGui::EndChild();
	});

	return true;
}

void ExampleFeature::HandleMenu()
{
	// Add the page button before the misc separator
	if (!m_pExampleFeatureButton->HasParent())
		Framework::menu->GetChild("SIDEBAR")->InsertElementAfter(m_pExampleFeatureButton.get(), "FEATURE_SEPERATOR");

	auto pHeaderGroup = static_cast<HeaderGroup*>(Framework::menu->GetChild("HEADER_GROUP"));
	if (pHeaderGroup)
	{
		static bool bHeadersAdded = false;
		if (!bHeadersAdded)
		{
			pHeaderGroup->AddHeaders(ExampleFeature::s_iExamplePageId, { "EXAMPLE_MAIN"Hashed, "EXAMPLE_SECONDARY"Hashed });
			bHeadersAdded = true;
		}
	}

	if (!m_pBodyGroup->HasParent())
		Framework::menu->GetChild("HEADER_GROUP")->GetChild("BODY")->AddElement(m_pBodyGroup.get());
}