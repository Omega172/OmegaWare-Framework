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
		{ "LANGUAGE"Hashed, "Language" },
		{ "UI_SCALE"Hashed, "UI Scale" },
		{ "GENERAL_GROUP"Hashed, "General" },
		{ "CONFIG_GROUP"Hashed, "Config" },

		{ "CONFIG_NAME_PLACEHOLDER"Hashed, "Enter a name..." },
		{ "CONFIG_SAVE"Hashed, "Save" },
		{ "CONFIG_RELOAD"Hashed, "Reload" },
		{ "CONFIG_RENAME"Hashed, "Rename" },
		{ "CONFIG_DELETE"Hashed, "Delete" },
		{ "CONFIG_SET_DEFAULT"Hashed, "Set Default" },
		{ "CONFIG_OPEN_FOLDER"Hashed, "Open Folder" },
		{ "CONFIG_DEFAULT_SUFFIX"Hashed, "(Default)" },
		{ "OPEN_LOGS_FOLDER"Hashed, "Open Logs Folder" },

		{ "SIDEBAR"Hashed, "Sidebar" },
		{ "FEATURE_SEPERATOR"Hashed, "Features" },
		{ "MISC_SEPERATOR"Hashed, "Misc" },
		{ "DEVELOPER_BUTTON"Hashed, "Developer" },
		{ "SETTINGS_BUTTON"Hashed, "Settings" },

		{ "DEVELOPER_MAIN"Hashed, "Main" },
		{ "SETTINGS_MAIN"Hashed, "Main" },
		{ "STYLE_BUTTON"Hashed, "Style" },
		{ "STYLE_MAIN"Hashed, "Main" },

		{ "RESET_STYLE"Hashed, "Reset to Default" },
		{ "COLOR_GROUP"Hashed, "Colors" },
		{ "COLOR_WINDOW_BG"Hashed, "Window Background" },
		{ "COLOR_CHILD_BG"Hashed, "Group Background" },
		{ "COLOR_CHROME_BG"Hashed, "Sidebar & Header" },
		{ "COLOR_HEADER"Hashed, "Tabs" },
		{ "COLOR_BORDER"Hashed, "Border" },
		{ "COLOR_TEXT"Hashed, "Text" },
		{ "COLOR_ACCENT"Hashed, "Accent" },
		{ "COLOR_BUTTON"Hashed, "Button" },

		{ "PREVIEW_GROUP"Hashed, "Preview" },
		{ "PREVIEW_BUTTON"Hashed, "Example Button" },
		{ "PREVIEW_ACCENT_BUTTON"Hashed, "Example Accent Button" },
		{ "PREVIEW_TOGGLE"Hashed, "Example Toggle" },
		{ "PREVIEW_CHECKBOX"Hashed, "Example Checkbox" },
		{ "PREVIEW_SLIDER"Hashed, "Example Slider" },
		{ "PREVIEW_COMBO"Hashed, "Example Combo" },
		{ "PREVIEW_TEXT"Hashed, "This is example text" },
		{ "PREVIEW_TABLE_COL_A"Hashed, "Column A" },
		{ "PREVIEW_TABLE_COL_B"Hashed, "Column B" },

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
		{ "EXAMPLE_SECONDARY_TEXT"Hashed, "This is some text on the secondary page." },
		{ "EXAMPLE_TABLE_COL_STAT"Hashed, "Stat" },
		{ "EXAMPLE_TABLE_COL_VALUE"Hashed, "Value" }
	}),
};
