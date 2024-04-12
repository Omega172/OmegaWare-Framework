#pragma once
#include "../Localization.h"

Locale_t localePolish{
	.sKey = "Polski",
	.ullKeyHash = "POL"Hashed,
	.hMenuFont = &TahomaFontPolish,
	.hFeatureFont = &TahomaFontPolishESP,
	.umLocalizedStrings = std::unordered_map<size_t, std::string>({
		{ "CHEAT"Hashed, "Oszustwo" },
		{ "UNLOAD_BTN"Hashed, "Wyłacz" },
		{ "CONSOLE_SHOW"Hashed, "Pokaż Konsole" },
		{ "CONSOLE_HIDE"Hashed, "Schowaj Konsole" },
		{ "WATER_MARK"Hashed, "Znak wodny" },
		{ "WATER_MARK_FPS"Hashed, "Widoczność FPS na Znaku Wodnym" },
		{ "SAVE_CONFIG"Hashed, "Zapisz Konfiguracje" },
		{ "LOAD_CONFIG"Hashed, "Wczytaj Konfiguracje" },
		{ "LANGUAGE"Hashed, "Język" },
	}),
};
