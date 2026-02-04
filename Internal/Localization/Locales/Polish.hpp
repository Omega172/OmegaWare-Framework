#pragma once
#include "../Localization.hpp"

Locale_t localePolish{
	.sKey = "Polski",
	.ullKeyHash = "POL"Hashed,
	.hMenuFont = &TahomaFont,
	.hFeatureFont = &TahomaFont,
	.umLocalizedStrings = std::unordered_map<size_t, std::string>({
		{ "CHEAT"Hashed, "Oszustwo" },
		{ "UNLOAD_BTN"Hashed, "Wyłacz" },
		{ "CONSOLE_SHOW"Hashed, "Pokaż Konsole" },
		{ "CONSOLE_HIDE"Hashed, "Schowaj Konsole" },
		{ "SAVE_CONFIG"Hashed, "Zapisz Konfiguracje" },
		{ "LOAD_CONFIG"Hashed, "Wczytaj Konfiguracje" },
		{ "LANGUAGE"Hashed, "Język" },
	}),
};
