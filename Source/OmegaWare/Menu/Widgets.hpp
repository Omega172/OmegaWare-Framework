#pragma once

#include "Config/Config.hpp"

#include <string_view>

namespace OmegaWare::Widgets
{
	bool Keybind(std::string_view strLabel, Config::stKeybind& rKeybind);
}
