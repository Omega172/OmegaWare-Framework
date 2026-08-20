#pragma once
#include "pch.h"

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

	inline constexpr ImVec2 kBaseMenuSize = ImVec2(1000.f, 650.f);

	inline float sWIDTH = float(GetSystemMetrics(SM_CXSCREEN));
	inline float sHEIGHT = float(GetSystemMetrics(SM_CYSCREEN));

	inline std::unique_ptr<Child> GuiSidebar = std::make_unique<Child>(std::string("SIDEBAR"), "SIDEBAR"Hashed, ElementBase::Style_t{
		.vec2Size = ImVec2(kSidebarWidth, 0.f), .iFlags = ImGuiChildFlags_Border }, ImGuiWindowFlags_NoBackground);
	inline std::unique_ptr<SeperatorText> GuiFeatureSeperator = std::make_unique<SeperatorText>(std::string("FEATURE_SEPERATOR"), "FEATURE_SEPERATOR"Hashed);
	inline std::unique_ptr<SeperatorText> GuiMiscSeperator = std::make_unique<SeperatorText>(std::string("MISC_SEPERATOR"), "MISC_SEPERATOR"Hashed);
	inline std::unique_ptr<RadioButtonIcon> GuiDeveloper = std::make_unique<RadioButtonIcon>(std::string("DEVELOPER_BUTTON"), "DEVELOPER_BUTTON"Hashed, ElementBase::Style_t{
		.vec2Size = ImVec2(-0.1f, 0) }, ICON_FA_TERMINAL, true);
	inline std::unique_ptr<RadioButtonIcon> GuiSettings = std::make_unique<RadioButtonIcon>(std::string("SETTINGS_BUTTON"), "SETTINGS_BUTTON"Hashed, ElementBase::Style_t{
		.vec2Size = ImVec2(-0.1f, 0) }, ICON_FA_GEAR, true);
	inline std::unique_ptr<RadioButtonIcon> GuiStyle = std::make_unique<RadioButtonIcon>(std::string("STYLE_BUTTON"), "STYLE_BUTTON"Hashed, ElementBase::Style_t{
		.vec2Size = ImVec2(-0.1f, 0) }, ICON_FA_PALETTE, true);

	inline std::vector<ElementBase::Header_t> HeaderGroupHeaders = {};

	inline std::unique_ptr<HeaderGroup> GuiHeaderGroup = std::make_unique<HeaderGroup>(std::string("HEADER_GROUP"), "HEADER_GROUP"Hashed, ElementBase::Style_t{
		.vec2Size = ImVec2(-0.1f, 0), }, HeaderGroupHeaders);

	inline std::unique_ptr<Body> GuiBody = std::make_unique<Body>(std::string("BODY"), ElementBase::Style_t{
		.vec2Size = ImVec2(-0.1f, 0), });

	inline std::unique_ptr<Page> GuiDeveloperPage = std::make_unique<Page>("DEVELOPER_PAGE", ElementBase::Style_t(), 0, 0);
	inline std::unique_ptr<Button> GuiConsoleVisibility = std::make_unique<Button>(std::string("CONSOLE_VISIBILITY"), "CONSOLE_HIDE"Hashed);
	inline std::unique_ptr<Button> GuiOpenLogsFolder = std::make_unique<Button>(std::string("OPEN_LOGS_FOLDER"), "OPEN_LOGS_FOLDER"Hashed);

	inline std::unique_ptr<Page> GuiSettingsPage = std::make_unique<Page>("SETTINGS_PAGE", ElementBase::Style_t(), 0, 0);

	inline std::unique_ptr<Button> GuiUnloadButton = std::make_unique<Button>(std::string("UNLOAD_BTN"), "UNLOAD_BTN"Hashed, ElementBase::Style_t{
		.vec2Size = ImVec2(-0.1f, 0) });

	inline std::unique_ptr<GroupChild> GuiSettingsLeftGroup = std::make_unique<GroupChild>(std::string("SETTINGS_LEFT_GROUP"), "GENERAL_GROUP"Hashed, ElementBase::Style_t{
		.iFlags = ImGuiChildFlags_Border }, ImGuiWindowFlags_NoScrollbar);

	inline std::unique_ptr<GroupChild> GuiConfigGroup = std::make_unique<GroupChild>(std::string("CONFIG_GROUP"), "CONFIG_GROUP"Hashed, ElementBase::Style_t{
		.eSameLine = ElementBase::ESameLine::Same, .iFlags = ImGuiChildFlags_Border }, ImGuiWindowFlags_NoScrollbar);

	inline std::unique_ptr<ConfigManager> GuiConfigManager = std::make_unique<ConfigManager>(std::string("CONFIG_MANAGER"));

	inline nlohmann::json jsonConfigDefaults{};

	inline std::unique_ptr<Combo> GuiLocalization = std::make_unique<Combo>(std::string("LANGUAGE"), "LANGUAGE"Hashed, ElementBase::Style_t{
		.eSameLine = ElementBase::ESameLine::Same, .iFlags = ImGuiComboFlags_WidthFitPreview });

	inline constexpr float kUIScaleMin = 1.0f;
	inline constexpr float kUIScaleMax = 2.0f;
	inline constexpr float kUIScaleStep = 0.25f;

	inline std::unique_ptr<PersistentCombo> GuiUIScale = std::make_unique<PersistentCombo>(std::string("UI_SCALE"), "UI_SCALE"Hashed, ElementBase::Style_t{
		.iFlags = ImGuiComboFlags_WidthFitPreview });

	inline std::unique_ptr<Page> GuiStylePage = std::make_unique<Page>("STYLE_PAGE", ElementBase::Style_t(), 0, 0);
	inline std::unique_ptr<AccentButton> GuiResetStyle = std::make_unique<AccentButton>(std::string("RESET_STYLE"), "RESET_STYLE"Hashed);

	inline std::unique_ptr<GroupChild> GuiColorGroup = std::make_unique<GroupChild>(std::string("COLOR_GROUP"), "COLOR_GROUP"Hashed, ElementBase::Style_t{
		.iFlags = ImGuiChildFlags_Border | ImGuiChildFlags_AutoResizeY }, ImGuiWindowFlags_NoScrollbar);
	inline std::unique_ptr<ColorPicker> GuiColorWindowBg = std::make_unique<ColorPicker>(std::string("COLOR_WINDOW_BG"), "COLOR_WINDOW_BG"Hashed);
	inline std::unique_ptr<ColorPicker> GuiColorChildBg = std::make_unique<ColorPicker>(std::string("COLOR_CHILD_BG"), "COLOR_CHILD_BG"Hashed);
	inline std::unique_ptr<ColorPicker> GuiColorChrome = std::make_unique<ColorPicker>(std::string("COLOR_CHROME_BG"), "COLOR_CHROME_BG"Hashed);
	inline std::unique_ptr<ColorPicker> GuiColorHeader = std::make_unique<ColorPicker>(std::string("COLOR_HEADER"), "COLOR_HEADER"Hashed);
	inline std::unique_ptr<ColorPicker> GuiColorBorder = std::make_unique<ColorPicker>(std::string("COLOR_BORDER"), "COLOR_BORDER"Hashed);
	inline std::unique_ptr<ColorPicker> GuiColorText = std::make_unique<ColorPicker>(std::string("COLOR_TEXT"), "COLOR_TEXT"Hashed);
	inline std::unique_ptr<ColorPicker> GuiColorAccent = std::make_unique<ColorPicker>(std::string("COLOR_ACCENT"), "COLOR_ACCENT"Hashed);
	inline std::unique_ptr<ColorPicker> GuiColorButton = std::make_unique<ColorPicker>(std::string("COLOR_BUTTON"), "COLOR_BUTTON"Hashed);

	inline std::unique_ptr<GroupChild> GuiPreviewGroup = std::make_unique<GroupChild>(std::string("PREVIEW_GROUP"), "PREVIEW_GROUP"Hashed, ElementBase::Style_t{
		.iFlags = ImGuiChildFlags_Border | ImGuiChildFlags_AutoResizeY }, ImGuiWindowFlags_NoScrollbar);
	inline std::unique_ptr<Button> GuiPreviewButton = std::make_unique<Button>(std::string("PREVIEW_BUTTON"), "PREVIEW_BUTTON"Hashed);
	inline std::unique_ptr<AccentButton> GuiPreviewAccentButton = std::make_unique<AccentButton>(std::string("PREVIEW_ACCENT_BUTTON"), "PREVIEW_ACCENT_BUTTON"Hashed, ElementBase::Style_t{
		.eSameLine = ElementBase::ESameLine::Same });
	inline std::unique_ptr<Toggle> GuiPreviewToggle = std::make_unique<Toggle>(std::string("PREVIEW_TOGGLE"), "PREVIEW_TOGGLE"Hashed);
	inline std::unique_ptr<Checkbox> GuiPreviewCheckbox = std::make_unique<Checkbox>(std::string("PREVIEW_CHECKBOX"), "PREVIEW_CHECKBOX"Hashed, ElementBase::Style_t{
		.eSameLine = ElementBase::ESameLine::Same });
	inline std::unique_ptr<SliderFloat> GuiPreviewSlider = std::make_unique<SliderFloat>(std::string("PREVIEW_SLIDER"), "PREVIEW_SLIDER"Hashed, ElementBase::Style_t(), 0.5f, 0.f, 1.f);
	inline std::unique_ptr<Combo> GuiPreviewCombo = std::make_unique<Combo>(std::string("PREVIEW_COMBO"), "PREVIEW_COMBO"Hashed);
	inline std::unique_ptr<_Text> GuiPreviewText = std::make_unique<_Text>(std::string("PREVIEW_TEXT"), "PREVIEW_TEXT"Hashed);
	inline std::unique_ptr<Table> GuiPreviewTable = std::make_unique<Table>(std::string("PREVIEW_TABLE"), 2);

	inline ImGuiStyle BaseStyle{};
	inline float fLastAppliedUIScale = 1.0f;

	inline bool bPendingFontRebuild = false;
	inline float flPendingUIScale = 1.0f;

	void Render();

	inline std::once_flag LoadFlag;
}
