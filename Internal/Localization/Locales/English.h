#pragma once
#include "../Localization.h"

Locale_t localeEnglish{
	.sKey = "English",
	.ullKeyHash = "ENG"Hashed,
	.hMenuFont = &TahomaFont,
	.hFeatureFont = &TahomaFontESP,
	.umLocalizedStrings = std::unordered_map<size_t, std::string>({
		{ "CHEAT"Hashed, "Cheat"},
		{ "UNLOAD_BTN"Hashed, "Unload" },
		{ "CONSOLE_SHOW"Hashed, "Show Console" },
		{ "CONSOLE_HIDE"Hashed, "Hide Console" },
		{ "WATER_MARK"Hashed, "Watermark" },
		{ "WATER_MARK_FPS"Hashed, "Watermark FPS" },
		{ "SAVE_CONFIG"Hashed, "Save Config" },
		{ "LOAD_CONFIG"Hashed, "Load Config" },
		{ "LANGUAGE"Hashed, "Language" },

		{ "SIDEBAR"Hashed, "Sidebar" },
		{ "MISC_SEPERATOR"Hashed, "Misc" },
		{ "DEVELOPER_BUTTON"Hashed, "Developer" },
		{ "STYLE_BUTTON"Hashed, "Style" },
		{ "SETTINGS_BUTTON"Hashed, "Settings" },
		{ "CONFIG_BUTTON"Hashed, "Config" },

		{ "MAIN"Hashed, "Main" },
		{ "EXAMPLE"Hashed, "Example" },
		{ "EXAMPLE2"Hashed, "Example2" }
	}),
};
