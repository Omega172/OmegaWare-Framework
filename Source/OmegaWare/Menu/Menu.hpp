#pragma once

#include <string>
#include <string_view>

namespace OmegaWare::Menu
{
	void Initialize();
	void Shutdown();
	void Draw(std::string_view strBackend);
	void ToggleConsole();
	[[nodiscard]] float ResolveScale(float flDisplayHeight);

	struct stHotkeys
	{
		int iMenu = 0;
		int iConsole = 0;
		int iUnload = 0;
	};

	[[nodiscard]] stHotkeys Hotkeys();
	[[nodiscard]] std::string HotkeyName(int iKey);
}
