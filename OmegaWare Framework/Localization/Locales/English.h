#pragma once
#include "../Localization.h"

Locale_t localeEnglish{
	.sKey = "English",
	.ullKeyHash = "ENG"_hash,
	.hMenuFont = &TahomaFont,
	.hFeatureFont = &TahomaFontESP,
	.umLocalizedStrings = std::unordered_map<size_t, std::string>({
		{ "CHEAT"_hash, "Cheat" },
		{ "UNLOAD_BTN"_hash, "Unload" },
		{ "CONSOLE_SHOW"_hash, "Show Console" },
		{ "CONSOLE_HIDE"_hash, "Hide Console" },
		{ "WATER_MARK"_hash, "Watermark" },
		{ "WATER_MARK_FPS"_hash, "Watermark FPS" },
		{ "SAVE_CONFIG"_hash, "Save Config" },
		{ "LOAD_CONFIG"_hash, "Load Config" },
		{ "LANGUAGE"_hash, "Language" },
	}),
};