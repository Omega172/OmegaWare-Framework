#pragma once
#include "../LocaleStructs.h"

std::vector<LocaleData> PolishLocales = {
	{"CHEAT"_hash, "Oszustwo"},
	{"UNLOAD_BTN"_hash, "Wyłacz"},
	{"CONSOLE_SHOW"_hash, "Pokaż Konsole"},
	{"CONSOLE_HIDE"_hash, "Schowaj Konsole"},
	{"WATER_MARK"_hash, "Znak wodny"},
	{"WATER_MARK_FPS"_hash, "Widoczność FPS na Znaku Wodnym"},
	{"SAVE_CONFIG"_hash, "Zapisz Konfiguracje"},
	{"LOAD_CONFIG"_hash, "Wczytaj Konfiguracje"},
	{"LANGUAGE"_hash, "Język"}
};

LocalizationData Polish{ "Polski", "POL"_hash, &TahomaFontPolish, &TahomaFontPolishESP, PolishLocales };