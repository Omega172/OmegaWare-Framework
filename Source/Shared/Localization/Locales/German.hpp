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
		{ "CONSOLE_SHOW"Hashed, "Konsole anzeigen" },
		{ "CONSOLE_HIDE"Hashed, "Konsole ausblenden" },
		{ "SAVE_CONFIG"Hashed, "Konfig speichern" },
		{ "LOAD_CONFIG"Hashed, "Konfig laden" },
		{ "LANGUAGE"Hashed, "Sprache" },
		{ "UI_SCALE"Hashed, "UI-Skalierung" },

		{ "SIDEBAR"Hashed, "Seitenleiste" },
		{ "FEATURE_SEPERATOR"Hashed, "Features" },
		{ "MISC_SEPERATOR"Hashed, "Sonstiges" },
		{ "DEVELOPER_BUTTON"Hashed, "Entwickler" },
		{ "CONFIG_BUTTON"Hashed, "Konfig" },

		{ "DEVELOPER_MAIN"Hashed, "Allgemein" },
		{ "CONFIG_MAIN"Hashed, "Allgemein" },

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
