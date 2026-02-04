#pragma once
#include "../Localization.hpp"

Locale_t localeEnglish{
	.sKey = "English",
	.ullKeyHash = "ENG"Hashed,
	.hMenuFont = &TahomaFont,
	.hFeatureFont = &TahomaFontFeature,
	.umLocalizedStrings = std::unordered_map<size_t, std::string>({
		{ "CHEAT"Hashed, "Cheat"},
		{ "UNLOAD_BTN"Hashed, "Unload" },
		{ "CONSOLE_SHOW"Hashed, "Show Console" },
		{ "CONSOLE_HIDE"Hashed, "Hide Console" },
		{ "SAVE_CONFIG"Hashed, "Save Config" },
		{ "LOAD_CONFIG"Hashed, "Load Config" },
		{ "LANGUAGE"Hashed, "Language" },

		{ "SIDEBAR"Hashed, "Sidebar" },
		{ "FEATURE_SEPERATOR"Hashed, "Features" },
		{ "MISC_SEPERATOR"Hashed, "Misc" },
		{ "DEVELOPER_BUTTON"Hashed, "Developer" },
		{ "CONFIG_BUTTON"Hashed, "Config" },

		{ "DEVELOPER_MAIN"Hashed, "Main" },
		{ "CONFIG_MAIN"Hashed, "Main" }
	}),
};
