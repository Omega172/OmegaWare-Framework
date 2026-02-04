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
	
	std::unique_ptr<BodyGroup> m_pBodyGroup = std::make_unique<BodyGroup>(
		"EXAMPLE_BODY_GROUP", 
		ElementBase::Style_t(), 
		s_iExamplePageId, 
		0);

public:
	bool SetupMenu();

	void HandleMenu();

	std::string GetName() { return "ExampleFeature"; };
};

std::unique_ptr<ExampleFeature> pExampleFeature = std::make_unique<ExampleFeature>();