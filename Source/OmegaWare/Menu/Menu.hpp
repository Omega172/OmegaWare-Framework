#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace OmegaWare::Menu
{
	void Initialize();
	void Shutdown();
	void Draw(std::string_view strBackend);
	void ToggleConsole();
	[[nodiscard]] float ResolveScale(float flDisplayHeight);
	using FnPluginThemeApply = void (*)(float flDeltaTime);
	void RegisterPluginTheme(std::string_view strName, FnPluginThemeApply pfnApply);
	void UnregisterPluginTheme(std::string_view strName);

	struct stHotkeys
	{
		int iMenu = 0;
		int iConsole = 0;
		int iUnload = 0;
	};

	[[nodiscard]] stHotkeys Hotkeys();
	[[nodiscard]] std::string HotkeyName(int iKey);
}
