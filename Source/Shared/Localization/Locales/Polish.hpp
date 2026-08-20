#pragma once
#include "../Localization.hpp"

Locale_t localePolish{
	.sKey = "Polski",
	.ullKeyHash = "POL"Hashed,
	.hMenuFont = &TahomaFont,
	.hFeatureFont = &TahomaFontFeature,
	.umLocalizedStrings = std::unordered_map<size_t, std::string>({
		{ "CHEAT"Hashed, "Cheat" },
		{ "UNLOAD_BTN"Hashed, "Wyładuj" },
		{ "CONSOLE_SHOW"Hashed, "Pokaż konsolę" },
		{ "CONSOLE_HIDE"Hashed, "Schowaj konsolę" },
		{ "LANGUAGE"Hashed, "Język" },
		{ "UI_SCALE"Hashed, "Skala UI" },
		{ "GENERAL_GROUP"Hashed, "Ogólne" },
		{ "CONFIG_GROUP"Hashed, "Config" },

		{ "CONFIG_NAME_PLACEHOLDER"Hashed, "Wpisz nazwę..." },
		{ "CONFIG_SAVE"Hashed, "Zapisz" },
		{ "CONFIG_RELOAD"Hashed, "Wczytaj ponownie" },
		{ "CONFIG_RENAME"Hashed, "Zmień nazwę" },
		{ "CONFIG_DELETE"Hashed, "Usuń" },
		{ "CONFIG_SET_DEFAULT"Hashed, "Ustaw jako domyślny" },
		{ "CONFIG_OPEN_FOLDER"Hashed, "Otwórz folder" },
		{ "CONFIG_DEFAULT_SUFFIX"Hashed, "(Domyślny)" },
		{ "OPEN_LOGS_FOLDER"Hashed, "Otwórz folder logów" },

		{ "SIDEBAR"Hashed, "Panel boczny" },
		{ "FEATURE_SEPERATOR"Hashed, "Funkcje" },
		{ "MISC_SEPERATOR"Hashed, "Różne" },
		{ "DEVELOPER_BUTTON"Hashed, "Deweloper" },
		{ "SETTINGS_BUTTON"Hashed, "Ustawienia" },

		{ "DEVELOPER_MAIN"Hashed, "Główna" },
		{ "SETTINGS_MAIN"Hashed, "Główna" },
		{ "STYLE_BUTTON"Hashed, "Styl" },
		{ "STYLE_MAIN"Hashed, "Główna" },

		{ "RESET_STYLE"Hashed, "Przywróć domyślne" },
		{ "COLOR_GROUP"Hashed, "Kolory" },
		{ "COLOR_WINDOW_BG"Hashed, "Tło okna" },
		{ "COLOR_CHILD_BG"Hashed, "Tło grupy" },
		{ "COLOR_CHROME_BG"Hashed, "Pasek boczny i nagłówek" },
		{ "COLOR_HEADER"Hashed, "Zakładki" },
		{ "COLOR_BORDER"Hashed, "Obramowanie" },
		{ "COLOR_TEXT"Hashed, "Tekst" },
		{ "COLOR_ACCENT"Hashed, "Akcent" },
		{ "COLOR_BUTTON"Hashed, "Przycisk" },

		{ "PREVIEW_GROUP"Hashed, "Podgląd" },
		{ "PREVIEW_BUTTON"Hashed, "Przykładowy przycisk" },
		{ "PREVIEW_ACCENT_BUTTON"Hashed, "Przykładowy przycisk akcentu" },
		{ "PREVIEW_TOGGLE"Hashed, "Przykładowy przełącznik" },
		{ "PREVIEW_CHECKBOX"Hashed, "Przykładowy checkbox" },
		{ "PREVIEW_SLIDER"Hashed, "Przykładowy suwak" },
		{ "PREVIEW_COMBO"Hashed, "Przykładowy wybór" },
		{ "PREVIEW_TEXT"Hashed, "To jest przykładowy tekst" },
		{ "PREVIEW_TABLE_COL_A"Hashed, "Kolumna A" },
		{ "PREVIEW_TABLE_COL_B"Hashed, "Kolumna B" },

		{ "EXAMPLE_FEATURE"Hashed, "Przykładowa Funkcja" },
		{ "EXAMPLE_FEATURE_HW"Hashed, "Cześć Świecie!" },
		{ "EXAMPLE_FEATURE_SLIDER"Hashed, "Przykładowy suwak" },
		{ "EXAMPLE_COLORPICKER"Hashed, "Przykładowy próbnik kolorów" },

		{ "EXAMPLE_FEATURE_BUTTON"Hashed, "Przykładowa Funkcja"},
		{ "EXAMPLE_MAIN"Hashed, "Główna" },
		{ "EXAMPLE_SECONDARY"Hashed, "Przykład" },

		{ "EXAMPLE_MAIN_PAGE_GROUP_CHILD"Hashed, "Grupa" },
		{ "EXAMPLE_CHECKBOX"Hashed, "Rejestruj wartości w konsoli" },
		{ "EXAMPLE_MAIN_PAGE_GROUP_CHILD2"Hashed, "Grupa 2" },
		{ "EXAMPLE_TEXT"Hashed, "To jest jakiś tekst" },
		{ "EXAMPLE_TOGGLE"Hashed, "Przełącznik" },

		{ "EXAMPLE_MAIN_PAGE_CHILD"Hashed, "Grupa 3" },
		{ "EXAMPLE_COMBO"Hashed, "Wybór" },
		{ "EXAMPLE_INPUTTEXT"Hashed, "Wprowadzanie tekstu" },
		{ "EXAMPLE_SLIDERINT"Hashed, "Suwak liczb całkowitych" },
		{ "EXAMPLE_SLIDERFLOAT"Hashed, "Suwak liczb zmiennoprzecinkowych" },
		{ "EXAMPLE_ACCENTBUTTON"Hashed, "Przycisk akcentu" },

		{ "EXAMPLE_SECONDARY_CHILD"Hashed, "Podstrona 2" },
		{ "EXAMPLE_SECONDARY_TEXT"Hashed, "To jest jakiś tekst na stronie drugorzędnej." },
		{ "EXAMPLE_TABLE_COL_STAT"Hashed, "Statystyka" },
		{ "EXAMPLE_TABLE_COL_VALUE"Hashed, "Wartość" }
	}),
};
