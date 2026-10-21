#pragma once

#include "OmegaRenderHook/Core/Log.hpp"

#include <string_view>

namespace OmegaWare::Console
{
	void SetTitle(std::string_view strTitle);
	void SetVisible(bool bVisible);
	void Toggle();
	[[nodiscard]] bool IsVisible();
	void Write(OmegaRenderHook::ELogLevel eLevel, std::string_view strLine);
	void Shutdown();
}
