#pragma once
#include "../Localization.hpp"

Locale_t localeGerman{
	.sKey = "Deutsch",
	.ullKeyHash = "GER"Hashed,
	.hMenuFont = &TahomaFont,
	.hFeatureFont = &TahomaFontFeature,
	.umLocalizedStrings = std::unordered_map<size_t, std::string>({
		{ "CHEAT"Hashed, "Cheat" },
		{ "UNLOAD_BTN"Hashed, "Entladen" },
		{ "CONSOLE_SHOW"Hashed, "Konsole Zeigen" },
		{ "CONSOLE_HIDE"Hashed, "Konsole Ausblenden" },
		{ "SAVE_CONFIG"Hashed, "Konfiguration Speichern" },
		{ "LOAD_CONFIG"Hashed, "Konfiguration Laden" },
		{ "LANGUAGE"Hashed, "Sprache" },

		{ "SIDEBAR"Hashed, "Seitenleiste" },
		{ "FEATURE_SEPERATOR"Hashed, "Funktionen" },
		{ "MISC_SEPERATOR"Hashed, "Sonstiges" },
		{ "DEVELOPER_BUTTON"Hashed, "Entwickler" },
		{ "CONFIG_BUTTON"Hashed, "Konfiguration" },

		{ "DEVELOPER_MAIN"Hashed, "Haupt" },
		{ "CONFIG_MAIN"Hashed, "Haupt" }
	}),
};
