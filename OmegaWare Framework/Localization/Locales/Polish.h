#pragma once
#include "../Localization.h"

Locale_t localePolish{
	.sKey = "Polski",
	.ullKeyHash = "POL"_hash,
	.hMenuFont = &TahomaFontPolish,
	.hFeatureFont = &TahomaFontPolishESP,
	.umLocalizedStrings = std::unordered_map<size_t, std::string>({
		{ "CHEAT"_hash, "Oszustwo" },
		{ "UNLOAD_BTN"_hash, "Wyłacz" },
		{ "CONSOLE_SHOW"_hash, "Pokaż Konsole" },
		{ "CONSOLE_HIDE"_hash, "Schowaj Konsole" },
		{ "WATER_MARK"_hash, "Znak wodny" },
		{ "WATER_MARK_FPS"_hash, "Widoczność FPS na Znaku Wodnym" },
		{ "SAVE_CONFIG"_hash, "Zapisz Konfiguracje" },
		{ "LOAD_CONFIG"_hash, "Wczytaj Konfiguracje" },
		{ "LANGUAGE"_hash, "Język" },
	}),
};