#pragma once
#include "../Localization.h"

Locale_t localePolish{
	.sKey = "Polski",
	.ullKeyHash = HashString("POL"),
	.hMenuFont = &TahomaFontPolish,
	.hFeatureFont = &TahomaFontPolishESP,
	.umLocalizedStrings = std::unordered_map<size_t, std::string>({
		{ HashString("CHEAT"), "Oszustwo" },
		{ HashString("UNLOAD_BTN"), "Wyłacz" },
		{ HashString("CONSOLE_SHOW"), "Pokaż Konsole" },
		{ HashString("CONSOLE_HIDE"), "Schowaj Konsole" },
		{ HashString("WATER_MARK"), "Znak wodny" },
		{ HashString("WATER_MARK_FPS"), "Widoczność FPS na Znaku Wodnym" },
		{ HashString("SAVE_CONFIG"), "Zapisz Konfiguracje" },
		{ HashString("LOAD_CONFIG"), "Wczytaj Konfiguracje" },
		{ HashString("LANGUAGE"), "Język" },
	}),
};