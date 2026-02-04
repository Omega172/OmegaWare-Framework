#pragma once
#include "pch.h"
class ExampleFeature : public BaseFeature
{
private:
	inline static uint8_t s_iExamplePageId = ElementBase::AddPage("EXAMPLE_FEATURE_BUTTON"Hashed, ICON_FA_STAR);
	
	std::unique_ptr<RadioButtonIcon> m_pExampleFeatureButton = std::make_unique<RadioButtonIcon>(
		std::string("EXAMPLE_FEATURE_BUTTON"), 
		"EXAMPLE_FEATURE_BUTTON"Hashed, 
		ElementBase::Style_t({ .vec2Size = ImVec2(-0.1f, 0) }), 
		ICON_FA_STAR, 
		s_iExamplePageId);
	
	std::unique_ptr<Page> m_pMainPage = std::make_unique<Page>(
		"EXAMPLE_MAIN_PAGE", 
		ElementBase::Style_t(), 
		s_iExamplePageId, 
		0);

	std::unique_ptr<Group> m_pMainPageGroup = std::make_unique<Group>(
		"EXAMPLE_MAIN_PAGE_GROUP", 
		ElementBase::Style_t());

	std::unique_ptr<GroupChild> m_pMainPageGroupChild = std::make_unique<GroupChild>(
		"EXAMPLE_MAIN_PAGE_GROUP_CHILD",
		"EXAMPLE_MAIN_PAGE_GROUP_CHILD"Hashed,
		ElementBase::Style_t(),
		ImGuiChildFlags_Border);

	std::unique_ptr<Checkbox> m_pExampleCheckbox = std::make_unique<Checkbox>(
		"EXAMPLE_CHECKBOX",
		"EXAMPLE_CHECKBOX"Hashed);

	std::unique_ptr<ColorPicker> m_pExampleColorPicker = std::make_unique<ColorPicker>(
		"EXAMPLE_COLORPICKER",
		"EXAMPLE_COLORPICKER"Hashed,
		ElementBase::Style_t{ .eSameLine = ElementBase::ESameLine::Same });

	std::unique_ptr<GroupChild> m_pMainPageGroupChild2 = std::make_unique<GroupChild>(
		"EXAMPLE_MAIN_PAGE_GROUP_CHILD2",
		"EXAMPLE_MAIN_PAGE_GROUP_CHILD2"Hashed,
		ElementBase::Style_t(),
		ImGuiChildFlags_Border);
	
	std::unique_ptr<_Text> m_pExampleText = std::make_unique<_Text>(
		"EXAMPLE_TEXT",
		"EXAMPLE_TEXT"Hashed,
		ElementBase::Style_t{});

	std::unique_ptr<Toggle> m_pExampleToggle = std::make_unique<Toggle>(
		"EXAMPLE_TOGGLE",
		"EXAMPLE_TOGGLE"Hashed);

	std::unique_ptr<GroupChild> m_pMainPageChild = std::make_unique<GroupChild>(
		"EXAMPLE_MAIN_PAGE_CHILD",
		"EXAMPLE_MAIN_PAGE_CHILD"Hashed,
		ElementBase::Style_t{ .eSameLine = ElementBase::ESameLine::Same, .vec2Size = ImVec2(0.f, 0.f) },
		ImGuiChildFlags_Border);

	std::unique_ptr<Combo> m_pExampleCombo = std::make_unique<Combo>(
		"EXAMPLE_COMBO",
		"EXAMPLE_COMBO"Hashed);

	//input text, slider int, slider float, seperator, AcentButton
	std::unique_ptr<InputText> m_pExampleInputText = std::make_unique<InputText>(
		"EXAMPLE_INPUTTEXT",
		"EXAMPLE_INPUTTEXT"Hashed,
		ElementBase::Style_t());

	std::unique_ptr<SliderInt> m_pExampleSliderInt = std::make_unique<SliderInt>(
		"EXAMPLE_SLIDERINT",
		"EXAMPLE_SLIDERINT"Hashed,
		ElementBase::Style_t(),
		0,
		100);

	std::unique_ptr<SliderFloat> m_pExampleSliderFloat = std::make_unique<SliderFloat>(
		"EXAMPLE_SLIDERFLOAT",
		"EXAMPLE_SLIDERFLOAT"Hashed,
		ElementBase::Style_t(),
		0.f,
		1.f);
	
	std::unique_ptr<Seperator> m_pExampleSeperator = std::make_unique<Seperator>(
		"EXAMPLE_SEPERATOR",
		ElementBase::Style_t());

	std::unique_ptr<AccentButton> m_pExampleAccentButton = std::make_unique<AccentButton>(
		"EXAMPLE_ACCENTBUTTON",
		"EXAMPLE_ACCENTBUTTON"Hashed,
		ElementBase::Style_t());

	std::unique_ptr<Page> m_pSecondaryPage = std::make_unique<Page>(
		"EXAMPLE_SECONDARY_PAGE",
		ElementBase::Style_t(),
		s_iExamplePageId,
		1); // Set to subpage 1

	std::unique_ptr<GroupChild> m_pSecondaryPageChild = std::make_unique<GroupChild>(
		"EXAMPLE_SECONDARY_CHILD",
		"EXAMPLE_SECONDARY_CHILD"Hashed,
		ElementBase::Style_t{ .vec2Size = ImVec2(0, 0) },
		ImGuiChildFlags_Border
	);

	std::unique_ptr<_Text> m_pSecondaryPageText = std::make_unique<_Text>(
		"EXAMPLE_SECONDARY_TEXT",
		"EXAMPLE_SECONDARY_TEXT"Hashed,
		ElementBase::Style_t{}
	);

public:
	bool SetupMenu();

	void HandleMenu();

	void Run();

	std::string GetName() { return "ExampleFeature"; };
};

std::unique_ptr<ExampleFeature> pExampleFeature = std::make_unique<ExampleFeature>();