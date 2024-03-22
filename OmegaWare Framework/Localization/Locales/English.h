#pragma once
#include "../LocaleStructs.h"

std::vector<LocaleData> EnglishLocales = {
	{"CHEAT"_hash, "Cheat"},
	{"UNLOAD_BTN"_hash, "Unload"},
	{"CONSOLE_SHOW"_hash, "Show Console"},
	{"CONSOLE_HIDE"_hash, "Hide Console"},
	{"WATER_MARK"_hash, "Watermark"},
	{"WATER_MARK_FPS"_hash, "Watermark FPS"},
	{"SAVE_CONFIG"_hash, "Save Config"},
	{"LOAD_CONFIG"_hash, "Load Config"},
	{"LANGUAGE"_hash, "Language"}
};

LocalizationData English{ "English", "ENG"_hash, &TahomaFont, &TahomaFontESP, EnglishLocales };