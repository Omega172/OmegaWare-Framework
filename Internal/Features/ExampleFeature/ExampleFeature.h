#pragma once
#include "pch.h"
#include "Includes.h"

class ExampleBodyGroup : public ElementBase
{
protected:
	uint8_t m_iPageId;
	uint8_t m_iSubPageId;

public:
	ExampleBodyGroup(std::string sUnique, Style_t stStyle = {}, uint8_t iPageId = 0, uint8_t iSubPageId = 0)
	{
		m_sUnique = sUnique;
		m_stStyle = stStyle;
		m_iPageId = iPageId;
		m_iSubPageId = iSubPageId;
	};
	
	constexpr EElementType GetType() const override
	{
		return EElementType::BodyGroup;
	};

	void Render() override
	{
		if (!m_stStyle.bVisible)
			return;

		if (m_iPageId != eCurrentPage || m_iSubPageId != eCurrentSubPage)
			return;

		float fGroupWidth = (ImGui::GetWindowWidth() - 10.0f - 10.0f * 2) / 2;
		float fGroupHeight = (ImGui::GetWindowHeight() - 10.0f - 10.0f * 2) / 2;

		ImGui::BeginGroup();
		{
			ImGui::BeginChild("group1", ImVec2(fGroupWidth, fGroupHeight), ImGuiChildFlags_Border);
			{
				ImGui::TextDisabled("GROUP1");

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
				ImGui::TextDisabled("GROUP2");

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
			ImGui::TextDisabled("GROUP3");
	   
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
	}
};

class ExampleFeature : public BaseFeature
{
private:
	std::unique_ptr<ExampleBodyGroup> m_pBodyGroup = std::make_unique<ExampleBodyGroup>("EXAMPLE_BODY_GROUP", ElementBase::Style_t(), ElementBase::EPage::Developer, 0);

public:
	bool SetupMenu();

	void HandleMenu();

	std::string GetName() { return "ExampleFeature"; };
};

std::unique_ptr<ExampleFeature> pExampleFeature = std::make_unique<ExampleFeature>();