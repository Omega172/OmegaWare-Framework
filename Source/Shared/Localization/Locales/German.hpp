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
		{ "LANGUAGE"Hashed, "Sprache" },
		{ "UI_SCALE"Hashed, "UI-Skalierung" },
		{ "GENERAL_GROUP"Hashed, "Allgemein" },
		{ "CONFIG_GROUP"Hashed, "Konfig" },

		{ "CONFIG_NAME_PLACEHOLDER"Hashed, "Namen eingeben..." },
		{ "CONFIG_SAVE"Hashed, "Speichern" },
		{ "CONFIG_RELOAD"Hashed, "Neu laden" },
		{ "CONFIG_RENAME"Hashed, "Umbenennen" },
		{ "CONFIG_DELETE"Hashed, "Löschen" },
		{ "CONFIG_SET_DEFAULT"Hashed, "Als Standard festlegen" },
		{ "CONFIG_OPEN_FOLDER"Hashed, "Ordner öffnen" },
		{ "CONFIG_DEFAULT_SUFFIX"Hashed, "(Standard)" },
		{ "OPEN_LOGS_FOLDER"Hashed, "Log-Ordner öffnen" },

		{ "SIDEBAR"Hashed, "Seitenleiste" },
		{ "FEATURE_SEPERATOR"Hashed, "Features" },
		{ "MISC_SEPERATOR"Hashed, "Sonstiges" },
		{ "DEVELOPER_BUTTON"Hashed, "Entwickler" },
		{ "SETTINGS_BUTTON"Hashed, "Einstellungen" },

		{ "DEVELOPER_MAIN"Hashed, "Allgemein" },
		{ "SETTINGS_MAIN"Hashed, "Allgemein" },
		{ "STYLE_BUTTON"Hashed, "Stil" },
		{ "STYLE_MAIN"Hashed, "Allgemein" },

		{ "RESET_STYLE"Hashed, "Auf Standard zurücksetzen" },
		{ "COLOR_GROUP"Hashed, "Farben" },
		{ "COLOR_WINDOW_BG"Hashed, "Fensterhintergrund" },
		{ "COLOR_CHILD_BG"Hashed, "Gruppenhintergrund" },
		{ "COLOR_CHROME_BG"Hashed, "Seitenleiste & Kopfzeile" },
		{ "COLOR_HEADER"Hashed, "Tabs" },
		{ "COLOR_BORDER"Hashed, "Rahmen" },
		{ "COLOR_TEXT"Hashed, "Text" },
		{ "COLOR_ACCENT"Hashed, "Akzent" },
		{ "COLOR_BUTTON"Hashed, "Schaltfläche" },

		{ "PREVIEW_GROUP"Hashed, "Vorschau" },
		{ "PREVIEW_BUTTON"Hashed, "Beispiel-Schaltfläche" },
		{ "PREVIEW_ACCENT_BUTTON"Hashed, "Beispiel-Akzent-Schaltfläche" },
		{ "PREVIEW_TOGGLE"Hashed, "Beispiel-Umschalter" },
		{ "PREVIEW_CHECKBOX"Hashed, "Beispiel-Kontrollkästchen" },
		{ "PREVIEW_SLIDER"Hashed, "Beispiel-Schieberegler" },
		{ "PREVIEW_COMBO"Hashed, "Beispiel-Auswahl" },
		{ "PREVIEW_TEXT"Hashed, "Dies ist ein Beispieltext" },
		{ "PREVIEW_TABLE_COL_A"Hashed, "Spalte A" },
		{ "PREVIEW_TABLE_COL_B"Hashed, "Spalte B" },

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
		{ "EXAMPLE_SECONDARY_TEXT"Hashed, "Dies ist ein Text auf der Sekundärseite." },
		{ "EXAMPLE_TABLE_COL_STAT"Hashed, "Statistik" },
		{ "EXAMPLE_TABLE_COL_VALUE"Hashed, "Wert" }
	}),
};
