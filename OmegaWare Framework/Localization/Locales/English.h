#pragma once
#include "../Localization.h"

Locale_t localeEnglish{
	.sKey = "English",
	.ullKeyHash = HashString("ENG"),
	.hMenuFont = &TahomaFont,
	.hFeatureFont = &TahomaFontESP,
	.umLocalizedStrings = std::unordered_map<size_t, std::string>({
		{ HashString("CHEAT"), "Cheat"},
		{ HashString("UNLOAD_BTN"), "Unload" },
		{ HashString("CONSOLE_SHOW"), "Show Console" },
		{ HashString("CONSOLE_HIDE"), "Hide Console" },
		{ HashString("WATER_MARK"), "Watermark" },
		{ HashString("WATER_MARK_FPS"), "Watermark FPS" },
		{ HashString("SAVE_CONFIG"), "Save Config" },
		{ HashString("LOAD_CONFIG"), "Load Config" },
		{ HashString("LANGUAGE"), "Language" },
	}),
};