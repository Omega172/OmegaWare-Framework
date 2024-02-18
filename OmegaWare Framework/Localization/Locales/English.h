#pragma once
#include "../LocaleStructs.h"

std::vector<LocaleData> EnglishLocales = {
	{std::hash<std::string>{}("CHEAT_BTN"), "Cheat"},
	{std::hash<std::string>{}("UNLOAD_BTN"), "Unload"},
	{std::hash<std::string>{}("CONSOLE_SHOW"), "Show Console"},
	{std::hash<std::string>{}("CONSOLE_HIDE"), "Hide Console"},
	{std::hash<std::string>{}("WATER_MARK"), "Watermark"},
	{std::hash<std::string>{}("WATER_MARK_FPS"), "Watermark FPS"},
	{std::hash<std::string>{}("SAVE_CONFIG"), "Save Config"},
	{std::hash<std::string>{}("LOAD_CONFIG"), "Load Config"},
};
LocalizationData English{ "English", std::hash<std::string>{}("ENG"), EnglishLocales };