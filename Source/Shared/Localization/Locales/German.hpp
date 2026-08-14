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
		{ "CONFIG_MAIN"Hashed, "Haupt" },

		{ "EXAMPLE_FEATURE"Hashed, "Beispiel Funktion" },
		{ "EXAMPLE_FEATURE_HW"Hashed, "Hallo Welt!" },
		{ "EXAMPLE_FEATURE_SLIDER"Hashed, "Beispiel-Schieberegler" },
		{ "EXAMPLE_COLORPICKER"Hashed, "Beispiel Farbwähler" },

		{ "EXAMPLE_FEATURE_BUTTON"Hashed, "Beispiel Funktion"},
		{ "EXAMPLE_MAIN"Hashed, "Haupt" },
		{ "EXAMPLE_SECONDARY"Hashed, "Sekundär" },

		{ "EXAMPLE_MAIN_PAGE_GROUP_CHILD"Hashed, "Gruppe" },
		{ "EXAMPLE_CHECKBOX"Hashed, "Werte in Konsole protokollieren" },
		{ "EXAMPLE_MAIN_PAGE_GROUP_CHILD2"Hashed, "Gruppe 2" },
		{ "EXAMPLE_TEXT"Hashed, "Dies ist ein Text" },
		{ "EXAMPLE_TOGGLE"Hashed, "Umschalten" },

		{ "EXAMPLE_MAIN_PAGE_CHILD"Hashed, "Gruppe 3" },
		{ "EXAMPLE_COMBO"Hashed, "Auswahl" },
		{ "EXAMPLE_INPUTTEXT"Hashed, "Texteingabe" },
		{ "EXAMPLE_SLIDERINT"Hashed, "Ganzzahl-Schieberegler" },
		{ "EXAMPLE_SLIDERFLOAT"Hashed, "Dezimal-Schieberegler" },
		{ "EXAMPLE_ACCENTBUTTON"Hashed, "Akzent-Schaltfläche" },

		{ "EXAMPLE_SECONDARY_CHILD"Hashed, "Unterseite 2" },
		{ "EXAMPLE_SECONDARY_TEXT"Hashed, "Dies ist ein Text auf der Sekundärseite." }
	}),
};
