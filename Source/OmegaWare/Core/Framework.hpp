#pragma once

#include <Windows.h>

#ifndef OMEGAWARE_PROXY
#define OMEGAWARE_PROXY 0
#endif

#ifndef OMEGAWARE_PROXY_DLL
#define OMEGAWARE_PROXY_DLL ""
#endif

#if __has_include("Version.hpp")
#include "Version.hpp"
#else
#define OMEGAWARE_NAME "OmegaWare"
#define OMEGAWARE_TITLE "omegaware"
#define OMEGAWARE_VERSION "0.0.0"
#define OMEGAWARE_GAME "ExampleGame"
#define OMEGAWARE_PROCESS ""
#endif

#include <filesystem>
#include <string_view>

namespace OmegaWare
{
	inline constexpr std::string_view kName = OMEGAWARE_NAME;
	inline constexpr std::string_view kTitle = OMEGAWARE_TITLE;
	inline constexpr std::string_view kProcess = OMEGAWARE_PROCESS;
	inline constexpr std::string_view kVersion = OMEGAWARE_VERSION;
	inline constexpr std::string_view kGame = OMEGAWARE_GAME;

	void Start(HMODULE hModule);
	void RequestUnload();
	[[nodiscard]] bool IsUnloadRequested();
	[[nodiscard]] std::filesystem::path DataDirectory();
}
