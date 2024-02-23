#pragma once
#include "../LocaleStructs.h"

std::vector<LocaleData> PolishLocales = {
	{HASH("CHEAT"), "Oszustwo"},
	{HASH("UNLOAD_BTN"), "Wyłacz"},
	{HASH("CONSOLE_SHOW"), "Pokaż Konsole"},
	{HASH("CONSOLE_HIDE"), "Schowaj Konsole"},
	{HASH("WATER_MARK"), "Znak wodny"},
	{HASH("WATER_MARK_FPS"), "Widoczność FPS na Znaku Wodnym"},
	{HASH("SAVE_CONFIG"), "Zapisz Konfiguracje"},
	{HASH("LOAD_CONFIG"), "Wczytaj Konfiguracje"},
	{HASH("LANGUAGE"), "Język"}
};

LocalizationData Polish{ "Polski", HASH("POL"), PolishRanges, PolishLocales };