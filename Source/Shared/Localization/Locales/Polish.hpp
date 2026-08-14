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
		{ "SAVE_CONFIG"Hashed, "Zapisz config" },
		{ "LOAD_CONFIG"Hashed, "Wczytaj config" },
		{ "LANGUAGE"Hashed, "Język" },
		{ "UI_SCALE"Hashed, "Skala UI" },

		{ "SIDEBAR"Hashed, "Panel boczny" },
		{ "FEATURE_SEPERATOR"Hashed, "Funkcje" },
		{ "MISC_SEPERATOR"Hashed, "Różne" },
		{ "DEVELOPER_BUTTON"Hashed, "Deweloper" },
		{ "CONFIG_BUTTON"Hashed, "Config" },

		{ "DEVELOPER_MAIN"Hashed, "Główna" },
		{ "CONFIG_MAIN"Hashed, "Główna" },

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
		{ "EXAMPLE_SECONDARY_TEXT"Hashed, "To jest jakiś tekst na stronie drugorzędnej." }
	}),
};
