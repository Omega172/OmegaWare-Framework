#include "Menu/Widgets.hpp"

#include "OmegaGUI/OmegaGUI.hpp"
#include "OmegaLocalization/OmegaLocalization.hpp"

namespace OmegaWare::Widgets
{
	bool Keybind(std::string_view strLabel, Config::stKeybind& rKeybind)
	{
		const OmegaGUI::stHotkeyText stText{
			.strAlwaysOn = OmegaLocalization::Get("keybind.always"),
			.strHold = OmegaLocalization::Get("keybind.hold"),
			.strToggle = OmegaLocalization::Get("keybind.toggle"),
			.strNotify = OmegaLocalization::Get("keybind.notify"),
			.strHint = OmegaLocalization::Get("keybind.hint"),
		};
		return OmegaGUI::Hotkey(strLabel, &rKeybind, stText);
	}
}
