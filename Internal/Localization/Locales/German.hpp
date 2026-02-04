#pragma once
#include "../Localization.hpp"

Locale_t localeGerman{
	.sKey = "Deutsch",
	.ullKeyHash = "GER"Hashed,
	.hMenuFont = &TahomaFont,
	.hFeatureFont = &TahomaFont,
	.umLocalizedStrings = std::unordered_map<size_t, std::string>({
		{ "CHEAT"Hashed, "Schummeln" },
		{ "UNLOAD_BTN"Hashed, "Freigeben" },
		{ "CONSOLE_SHOW"Hashed, "Konsole Zeigen" },
		{ "CONSOLE_HIDE"Hashed, "Konsole Ausblenden" },
		{ "SAVE_CONFIG"Hashed, "Konfiguration Speichern" },
		{ "LOAD_CONFIG"Hashed, "Konfiguration Laden" },
		{ "LANGUAGE"Hashed, "Sprache" },
	}),
};
