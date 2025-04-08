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
	inline std::unique_ptr<SeperatorText> GuiMiscSeperator = std::make_unique<SeperatorText>(std::string("MISC_SEPERATOR"), "MISC_SEPERATOR"Hashed);
	inline std::unique_ptr<RadioButtonIcon> GuiDeveloper = std::make_unique<RadioButtonIcon>(std::string("DEVELOPER_BUTTON"), "DEVELOPER_BUTTON"Hashed, ElementBase::Style_t{
		.vec2Size = ImVec2(-0.1f, 0) }, ICON_FA_TERMINAL, ElementBase::EPage::Developer);
	inline std::unique_ptr<RadioButtonIcon> GuiStyle = std::make_unique<RadioButtonIcon>(std::string("STYLE_BUTTON"), "STYLE_BUTTON"Hashed, ElementBase::Style_t{
		.vec2Size = ImVec2(-0.1f, 0) }, ICON_FA_PALLET, ElementBase::EPage::Style);
	inline std::unique_ptr<RadioButtonIcon> GuiSettings = std::make_unique<RadioButtonIcon>(std::string("SETTINGS_BUTTON"), "SETTINGS_BUTTON"Hashed, ElementBase::Style_t{
		.vec2Size = ImVec2(-0.1f, 0) }, ICON_FA_GEAR, ElementBase::EPage::Settings);
	inline std::unique_ptr<RadioButtonIcon> GuiConfig = std::make_unique<RadioButtonIcon>(std::string("CONFIG_BUTTON"), "CONFIG_BUTTON"Hashed, ElementBase::Style_t{
		.vec2Size = ImVec2(-0.1f, 0) }, ICON_FA_FLOPPY_DISK, ElementBase::EPage::Config);
	
	inline std::vector<ElementBase::Header_t> HeaderGroupHeaders = {
		{ ElementBase::EPage::Developer, { "MAIN"Hashed, "EXAMPLE"Hashed, "EXAMPLE2"Hashed } }
	};
	inline std::unique_ptr<HeaderGroup> GuiHeaderGroup = std::make_unique<HeaderGroup>(std::string("HEADER_GROUP"), "HEADER_GROUP"Hashed, ElementBase::Style_t{
		.vec2Size = ImVec2(-0.1f, 0), }, HeaderGroupHeaders);

	inline std::unique_ptr<Body> GuiBody = std::make_unique<Body>(std::string("BODY"), ElementBase::Style_t{
		.vec2Size = ImVec2(-0.1f, 0), });

	/*
	inline std::unique_ptr<Child> GuiCheat = std::make_unique<Child>(std::string("CHEAT"), "CHEAT"Hashed, ElementBase::Style_t{
		.iFlags = ImGuiChildFlags_Border | ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY }, ImGuiWindowFlags_HorizontalScrollbar);
	inline std::unique_ptr<Spacing> GuiCheatSpacing1 = std::make_unique<Spacing>(std::string("SPACING_1"), "SPACING_1"Hashed);
	inline std::unique_ptr<Button> GuiUnloadButton = std::make_unique<Button>(std::string("UNLOAD_BTN"), "UNLOAD_BTN"Hashed);
	inline std::unique_ptr<Button> GuiConsoleVisibility = std::make_unique<Button>(std::string("CONSOLE_VISIBILITY"), "CONSOLE_HIDE"Hashed, ElementBase::Style_t{
		.eSameLine = ElementBase::ESameLine::Same });
	inline std::unique_ptr<Combo> GuiLocalization = std::make_unique<Combo>(std::string("LANGUAGE"), "LANGUAGE"Hashed, ElementBase::Style_t{
		.iFlags = ImGuiComboFlags_WidthFitPreview });
	inline std::unique_ptr<Checkbox> GuiWatermark = std::make_unique<Checkbox>(std::string("WATER_MARK"), "WATER_MARK"Hashed);
	inline std::unique_ptr<Checkbox> GuiWatermarkFPS = std::make_unique<Checkbox>(std::string("WATER_MARK_FPS"), "WATER_MARK_FPS"Hashed);
	inline std::unique_ptr<Button> GuiSaveConfig = std::make_unique<Button>(std::string("SAVE_CONFIG"), "SAVE_CONFIG"Hashed);
	inline std::unique_ptr<Button> GuiLoadConfig = std::make_unique<Button>(std::string("LOAD_CONFIG"), "LOAD_CONFIG"Hashed, ElementBase::Style_t{
		.eSameLine = ElementBase::ESameLine::Same });
	*/

	void Render();

	inline std::once_flag LoadFlag;
}