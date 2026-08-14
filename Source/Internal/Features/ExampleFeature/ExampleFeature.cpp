#include "pch.h"
#include "ExampleFeature.hpp"

bool ExampleFeature::SetupMenu()
{
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