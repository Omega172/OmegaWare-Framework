#pragma once
#include "../LocaleStructs.h"

std::vector<LocaleData> GermanLocales = {
	{HASH("CHEAT"), "Schummeln"},
	{HASH("UNLOAD_BTN"), "Freigeben"},
	{HASH("CONSOLE_SHOW"), "Konsole Zeigen"},
	{HASH("CONSOLE_HIDE"), "Konsole Ausblenden"},
	{HASH("WATER_MARK"), "Wasserzeichen"},
	{HASH("WATER_MARK_FPS"), "Wasserzeichen FPS"},
	{HASH("SAVE_CONFIG"), "Konfiguration Speichern"},
	{HASH("LOAD_CONFIG"), "Konfiguration Laden"},
	{HASH("LANGUAGE"), "Sprache"}
};
LocalizationData German{ "German", HASH("GER"), GermanLocales };