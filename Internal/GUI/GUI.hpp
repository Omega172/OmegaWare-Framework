#pragma once
#include "pch.h"
#include "Menu/Menu.hpp"

// Colors for ImGui
inline ImU32 Black = ImGui::ColorConvertFloat4ToU32({ 0.f, 0.f, 0.f, 1.f });
inline ImU32 White = ImGui::ColorConvertFloat4ToU32({ 1.f, 1.f, 1.f, 1.f });

inline ImU32 Red = ImGui::ColorConvertFloat4ToU32({ 1.f, 0.f, 0.f, 1.f });
inline ImU32 Green = ImGui::ColorConvertFloat4ToU32({ 0.f, 1.f, 0.f, 1.f });
inline ImU32 Blue = ImGui::ColorConvertFloat4ToU32({ 0.f, 0.f, 1.f, 1.f });

inline ImU32 Cyan = ImGui::ColorConvertFloat4ToU32({ 0.f, 1.f, 1.f, 1.f });
inline ImU32 Gold = ImGui::ColorConvertFloat4ToU32({ 1.f, .84f, 0.f, 1.f });
inline ImU32 Orange = ImGui::ColorConvertFloat4ToU32({ 1.f, .65f, 0.f, 1.f });
inline ImU32 Purple = ImGui::ColorConvertFloat4ToU32({ .5f, 0.f, .5f, 1.f });
inline ImU32 Magenta = ImGui::ColorConvertFloat4ToU32({ 1.f, 0.f, 1.f, 1.f });

inline ImU32 Gray = ImGui::ColorConvertFloat4ToU32({ .8f, .8f, .8f, 1.f });

namespace GUI
{
	inline bool bMenuOpen = true;

	inline float sWIDTH = float(GetSystemMetrics(SM_CXSCREEN));
	inline float sHEIGHT = float(GetSystemMetrics(SM_CYSCREEN));

	inline std::unique_ptr<Child> GuiSidebar = std::make_unique<Child>(std::string("SIDEBAR"), "SIDEBAR"Hashed, ElementBase::Style_t{
		.vec2Size = ImVec2(160.f, 0.f), .iFlags = ImGuiChildFlags_Border }, ImGuiWindowFlags_NoBackground);
	inline std::unique_ptr<SeperatorText> GuiFeatureSeperator = std::make_unique<SeperatorText>(std::string("FEATURE_SEPERATOR"), "FEATURE_SEPERATOR"Hashed);
	inline std::unique_ptr<SeperatorText> GuiMiscSeperator = std::make_unique<SeperatorText>(std::string("MISC_SEPERATOR"), "MISC_SEPERATOR"Hashed);
	inline std::unique_ptr<RadioButtonIcon> GuiDeveloper = std::make_unique<RadioButtonIcon>(std::string("DEVELOPER_BUTTON"), "DEVELOPER_BUTTON"Hashed, ElementBase::Style_t{
		.vec2Size = ImVec2(-0.1f, 0) }, ICON_FA_TERMINAL, true);
	inline std::unique_ptr<RadioButtonIcon> GuiConfig = std::make_unique<RadioButtonIcon>(std::string("CONFIG_BUTTON"), "CONFIG_BUTTON"Hashed, ElementBase::Style_t{
		.vec2Size = ImVec2(-0.1f, 0) }, ICON_FA_FLOPPY_DISK, true);
	
	// Headers are now added dynamically in GUI.cpp
	inline std::vector<ElementBase::Header_t> HeaderGroupHeaders = {};

	inline std::unique_ptr<HeaderGroup> GuiHeaderGroup = std::make_unique<HeaderGroup>(std::string("HEADER_GROUP"), "HEADER_GROUP"Hashed, ElementBase::Style_t{
		.vec2Size = ImVec2(-0.1f, 0), }, HeaderGroupHeaders);

	inline std::unique_ptr<Body> GuiBody = std::make_unique<Body>(std::string("BODY"), ElementBase::Style_t{
		.vec2Size = ImVec2(-0.1f, 0), });

	// Developer Page Elements
	inline std::unique_ptr<Page> GuiDeveloperPage = std::make_unique<Page>("DEVELOPER_PAGE", ElementBase::Style_t(), 0, 0);
	inline std::unique_ptr<Button> GuiUnloadButton = std::make_unique<Button>(std::string("UNLOAD_BTN"), "UNLOAD_BTN"Hashed);
	inline std::unique_ptr<Button> GuiConsoleVisibility = std::make_unique<Button>(std::string("CONSOLE_VISIBILITY"), "CONSOLE_HIDE"Hashed, ElementBase::Style_t{
		.eSameLine = ElementBase::ESameLine::Same });
	inline std::unique_ptr<Combo> GuiLocalization = std::make_unique<Combo>(std::string("LANGUAGE"), "LANGUAGE"Hashed, ElementBase::Style_t{
		.iFlags = ImGuiComboFlags_WidthFitPreview });

	// Config Page Elements
	inline std::unique_ptr<Page> GuiConfigPage = std::make_unique<Page>("CONFIG_PAGE", ElementBase::Style_t(), 0, 0);
	inline std::unique_ptr<Button> GuiSaveConfig = std::make_unique<Button>(std::string("SAVE_CONFIG"), "SAVE_CONFIG"Hashed);
	inline std::unique_ptr<Button> GuiLoadConfig = std::make_unique<Button>(std::string("LOAD_CONFIG"), "LOAD_CONFIG"Hashed, ElementBase::Style_t{
		.eSameLine = ElementBase::ESameLine::Same });

	void Render();

	inline std::once_flag LoadFlag;
}