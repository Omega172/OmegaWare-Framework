#pragma once
#include "../LocaleStructs.h"

std::vector<LocaleData> EnglishLocales = {
	{"CHEAT_BTN", "Cheat"},
	{"UNLOAD_BTN", "Unload"},
	{"CONSOLE_SHOW", "Show Console"},
	{"CONSOLE_HIDE", "Hide Console"},
	{"WATER_MARK", "Watermark"},
	{"WATER_MARK_FPS", "Watermark FPS"},
	{"SAVE_CONFIG", "Save Config"},
	{"LOAD_CONFIG", "Load Config"},
};
LocalizationData English{ "English", "ENG", EnglishLocales };