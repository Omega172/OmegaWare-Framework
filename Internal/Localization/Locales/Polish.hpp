#pragma once
#include "../Localization.hpp"

Locale_t localePolish{
	.sKey = "Polski",
	.ullKeyHash = "POL"Hashed,
	.hMenuFont = &TahomaFont,
	.hFeatureFont = &TahomaFontFeature,
	.umLocalizedStrings = std::unordered_map<size_t, std::string>({
		{ "CHEAT"Hashed, "Cheat" },
		{ "UNLOAD_BTN"Hashed, "Wyładuj" },
		{ "CONSOLE_SHOW"Hashed, "Pokaż Konsolę" },
		{ "CONSOLE_HIDE"Hashed, "Schowaj Konsolę" },
		{ "SAVE_CONFIG"Hashed, "Zapisz Konfigurację" },
		{ "LOAD_CONFIG"Hashed, "Wczytaj Konfigurację" },
		{ "LANGUAGE"Hashed, "Język" },

		{ "SIDEBAR"Hashed, "Panel Boczny" },
		{ "FEATURE_SEPERATOR"Hashed, "Funkcje" },
		{ "MISC_SEPERATOR"Hashed, "Różne" },
		{ "DEVELOPER_BUTTON"Hashed, "Deweloper" },
		{ "CONFIG_BUTTON"Hashed, "Konfiguracja" },

		{ "DEVELOPER_MAIN"Hashed, "Główna" },
		{ "CONFIG_MAIN"Hashed, "Główna" }
	}),
};
