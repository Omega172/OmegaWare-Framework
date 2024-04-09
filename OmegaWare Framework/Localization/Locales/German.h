#pragma once
#include "../Localization.h"

Locale_t localeGerman{
	.sKey = "Deutsch",
	.ullKeyHash = HashString("GER"),
	.hMenuFont = &TahomaFont,
	.hFeatureFont = &TahomaFontESP,
	.umLocalizedStrings = std::unordered_map<size_t, std::string>({
		{ HashString("CHEAT"), "Schummeln" },
		{ HashString("UNLOAD_BTN"), "Freigeben" },
		{ HashString("CONSOLE_SHOW"), "Konsole Zeigen" },
		{ HashString("CONSOLE_HIDE"), "Konsole Ausblenden" },
		{ HashString("WATER_MARK"), "Wasserzeichen" },
		{ HashString("WATER_MARK_FPS"), "Wasserzeichen FPS" },
		{ HashString("SAVE_CONFIG"), "Konfiguration Speichern" },
		{ HashString("LOAD_CONFIG"), "Konfiguration Laden" },
		{ HashString("LANGUAGE"), "Sprache" },
	}),
};