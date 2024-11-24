#pragma once
#include "../Localization.h"

Locale_t localeGerman{
	.sKey = "Deutsch",
	.ullKeyHash = "GER"Hashed,
	.hMenuFont = &TahomaFont,
	.hFeatureFont = &TahomaFontESP,
	.umLocalizedStrings = std::unordered_map<size_t, std::string>({
		{ "CHEAT"Hashed, "Schummeln" },
		{ "UNLOAD_BTN"Hashed, "Freigeben" },
		{ "CONSOLE_SHOW"Hashed, "Konsole Zeigen" },
		{ "CONSOLE_HIDE"Hashed, "Konsole Ausblenden" },
		{ "WATER_MARK"Hashed, "Wasserzeichen" },
		{ "WATER_MARK_FPS"Hashed, "Wasserzeichen FPS" },
		{ "SAVE_CONFIG"Hashed, "Konfiguration Speichern" },
		{ "LOAD_CONFIG"Hashed, "Konfiguration Laden" },
		{ "LANGUAGE"Hashed, "Sprache" },
	}),
};
