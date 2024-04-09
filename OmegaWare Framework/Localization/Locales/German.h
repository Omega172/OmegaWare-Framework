#pragma once
#include "../Localization.h"

Locale_t localeGerman{
	.sKey = "Deutsch",
	.ullKeyHash = "GER"_hash,
	.hMenuFont = &TahomaFont,
	.hFeatureFont = &TahomaFontESP,
	.umLocalizedStrings = std::unordered_map<size_t, std::string>({
		{ "CHEAT"_hash, "Schummeln" },
		{ "UNLOAD_BTN"_hash, "Freigeben" },
		{ "CONSOLE_SHOW"_hash, "Konsole Zeigen" },
		{ "CONSOLE_HIDE"_hash, "Konsole Ausblenden" },
		{ "WATER_MARK"_hash, "Wasserzeichen" },
		{ "WATER_MARK_FPS"_hash, "Wasserzeichen FPS" },
		{ "SAVE_CONFIG"_hash, "Konfiguration Speichern" },
		{ "LOAD_CONFIG"_hash, "Konfiguration Laden" },
		{ "LANGUAGE"_hash, "Sprache" },
	}),
};