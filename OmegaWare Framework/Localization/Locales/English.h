#pragma once
#include "../LocaleStructs.h"

std::vector<LocaleData> EnglishLocales = {
	{HASH("CHEAT"), "Cheat"},
	{HASH("UNLOAD_BTN"), "Unload"},
	{HASH("CONSOLE_SHOW"), "Show Console"},
	{HASH("CONSOLE_HIDE"), "Hide Console"},
	{HASH("WATER_MARK"), "Watermark"},
	{HASH("WATER_MARK_FPS"), "Watermark FPS"},
	{HASH("SAVE_CONFIG"), "Save Config"},
	{HASH("LOAD_CONFIG"), "Load Config"},
	{HASH("LANGUAGE"), "Language"}
};
LocalizationData English{ "English", HASH("ENG"), EnglishLocales };