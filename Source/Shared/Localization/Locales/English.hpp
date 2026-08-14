#pragma once
#include "../Localization.hpp"

Locale_t localeEnglish{
	.sKey = "English",
	.ullKeyHash = "ENG"Hashed,
	.hMenuFont = &TahomaFont,
	.hFeatureFont = &TahomaFontFeature,
	.umLocalizedStrings = std::unordered_map<size_t, std::string>({
		{ "CHEAT"Hashed, "Cheat"},
		{ "UNLOAD_BTN"Hashed, "Unload" },
		{ "CONSOLE_SHOW"Hashed, "Show Console" },
		{ "CONSOLE_HIDE"Hashed, "Hide Console" },
		{ "SAVE_CONFIG"Hashed, "Save Config" },
		{ "LOAD_CONFIG"Hashed, "Load Config" },
		{ "LANGUAGE"Hashed, "Language" },

		{ "SIDEBAR"Hashed, "Sidebar" },
		{ "FEATURE_SEPERATOR"Hashed, "Features" },
		{ "MISC_SEPERATOR"Hashed, "Misc" },
		{ "DEVELOPER_BUTTON"Hashed, "Developer" },
		{ "CONFIG_BUTTON"Hashed, "Config" },

		{ "DEVELOPER_MAIN"Hashed, "Main" },
		{ "CONFIG_MAIN"Hashed, "Main" },

		{ "EXAMPLE_FEATURE"Hashed, "Example Feature" },
		{ "EXAMPLE_FEATURE_HW"Hashed, "Hello, World!" },
		{ "EXAMPLE_FEATURE_SLIDER"Hashed, "Example Slider" },
		{ "EXAMPLE_COLORPICKER"Hashed, "Example Color Picker" },

		{ "EXAMPLE_FEATURE_BUTTON"Hashed, "Example Feature"},
		{ "EXAMPLE_MAIN"Hashed, "Main" },
		{ "EXAMPLE_SECONDARY"Hashed, "Example" },

		{ "EXAMPLE_MAIN_PAGE_GROUP_CHILD"Hashed, "Group" },
		{ "EXAMPLE_CHECKBOX"Hashed, "Log Values To Console" },
		{ "EXAMPLE_MAIN_PAGE_GROUP_CHILD2"Hashed, "Group 2" },
		{ "EXAMPLE_TEXT"Hashed, "This is some text" },
		{ "EXAMPLE_TOGGLE"Hashed, "Toggle" },

		{ "EXAMPLE_MAIN_PAGE_CHILD"Hashed, "Group 3" },
		{ "EXAMPLE_COMBO"Hashed, "Combo" },
		{ "EXAMPLE_INPUTTEXT"Hashed, "Input Text" },
		{ "EXAMPLE_SLIDERINT"Hashed, "Slider Int" },
		{ "EXAMPLE_SLIDERFLOAT"Hashed, "Slider Float" },
		{ "EXAMPLE_ACCENTBUTTON"Hashed, "Accent Button" },

		{ "EXAMPLE_SECONDARY_CHILD"Hashed, "Subpage 2" },
		{ "EXAMPLE_SECONDARY_TEXT"Hashed, "This is some text on the secondary page." }
	}),
};
