#include "Menu/Widgets.hpp"

#include "OmegaGUI/OmegaGUI.hpp"
#include "OmegaLocalization/OmegaLocalization.hpp"

#include <array>
#include <format>

namespace OmegaWare::Widgets
{
	bool Keybind(std::string_view strLabel, std::string_view strID, Config::stKeybind& rKeybind)
	{
		bool bChanged = OmegaGUI::Hotkey(strLabel, &rKeybind.uKey);

		const std::array<std::string_view, 3> aModes = {
			OmegaLocalization::Get("keybind.always"),
			OmegaLocalization::Get("keybind.hold"),
			OmegaLocalization::Get("keybind.toggle"),
		};
		int iMode = static_cast<int>(rKeybind.eMode);
		if (OmegaGUI::Combo(std::format("{}###{}.mode", OmegaLocalization::Get("keybind.mode"), strID), &iMode, aModes))
		{
			rKeybind.eMode = static_cast<Config::EKeybindMode>(iMode);
			rKeybind.bActive = rKeybind.eMode == Config::EKeybindMode::AlwaysOn;
			bChanged = true;
		}
		return bChanged;
	}
}
