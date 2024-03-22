#pragma once
#include "pch.h"
#include "Menu/Menu.h"

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
inline ImU32 Gray = ImGui::ColorConvertFloat4ToU32({ .8f, .8f, .8f, 1.f});

namespace GUI
{
	inline bool bMenuOpen = false;
	constexpr float WIDTH = 900;
	constexpr float HEIGHT = 400;

	inline float sWIDTH = float(GetSystemMetrics(SM_CXSCREEN));
	inline float sHEIGHT = float(GetSystemMetrics(SM_CYSCREEN));


	inline std::unique_ptr<Child> guiCheat = std::make_unique<Child>([]() { return ImVec2(ImGui::GetContentRegionAvail().x / 3, ImGui::GetContentRegionAvail().y / 2); });
	inline std::unique_ptr<Text> guiCheatText = std::make_unique<Text>();
	inline std::unique_ptr<Spacing> guiCheatSpacing1 = std::make_unique<Spacing>();
	inline std::unique_ptr<Button> guiUnloadButton = std::make_unique<Button>();
	inline std::unique_ptr<Button> guiConsoleVisibility = std::make_unique<Button>();
	inline std::unique_ptr<Combo> guiLocalization = std::make_unique<Combo>("Cheat::CurrentLocale.Name");
	inline std::unique_ptr<Checkbox> guiWatermark = std::make_unique<Checkbox>();
	inline std::unique_ptr<Checkbox> guiWatermarkFPS = std::make_unique<Checkbox>();
	inline std::unique_ptr<Button> guiSaveConfig = std::make_unique<Button>();
	inline std::unique_ptr<Button> guiLoadConfig = std::make_unique<Button>();


	void Render();
}