#pragma once
#include "../LocaleStructs.h"

std::vector<LocaleData> GermanLocales = {
	{"CHEAT"_hash, "Schummeln"},
	{"UNLOAD_BTN"_hash, "Freigeben"},
	{"CONSOLE_SHOW"_hash, "Konsole Zeigen"},
	{"CONSOLE_HIDE"_hash, "Konsole Ausblenden"},
	{"WATER_MARK"_hash, "Wasserzeichen"},
	{"WATER_MARK_FPS"_hash, "Wasserzeichen FPS"},
	{"SAVE_CONFIG"_hash, "Konfiguration Speichern"},
	{"LOAD_CONFIG"_hash, "Konfiguration Laden"},
	{"LANGUAGE"_hash, "Sprache"}
};

LocalizationData German{ "Deutsch", "GER"_hash, &TahomaFont, &TahomaFontESP, GermanLocales };