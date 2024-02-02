// pch.h: This is a precompiled header file.
// Files listed below are compiled only once, improving build performance for future builds.
// This also affects IntelliSense performance, including code completion and many code browsing features.
// However, files listed here are ALL re-compiled if any one of them is updated between builds.
// Do not add files here that you will be updating frequently as this negates the performance advantage.

#define FRAMEWORK_VERSION 1.0.0
#define FRAMEWORK_MAJOR_VERSION 1
#define FRAMEWORK_MINOR_VERSION 0
#define FRAMEWORK_REWORK_VERSION 0

#define FRAMEWORK_CODENAME "OmegaWare"
#define FRAMEWORK_TARGET_GAME ""
#pragma warning(disable : 5056)
static_assert(FRAMEWORK_TARGET_GAME != ""); // Make sure the target game is set

#define FRAMEWORK_UNREAL 0

#define FRAMEWORK_RENDER_D3D11 1
#define FRAMEWORK_RENDER_D3D12 0
static_assert(FRAMEWORK_RENDER_D3D11 != FRAMEWORK_RENDER_D3D12); // Don't allow both rendering frameworks

#if FRAMEWORK_RENDER_D3D11
#define KIERO_INCLUDE_D3D11  1
#define KIERO_INCLUDE_D3D12  0
#endif

#if FRAMEWORK_RENDER_D3D12
#define KIERO_INCLUDE_D3D11  0
#define KIERO_INCLUDE_D3D12  1
#endif

#define CurrentLoc std::source_location::current()

#ifndef PCH_H
#define PCH_H

#if FRAMEWORK_UNREAL
#include "SDK.h"

// Remember to incude these files in the project
// BasicTypes_Package.cpp
// CoreUObject_Package.cpp
// Engine_Package.cpp
#endif

#include <thread>
#include <chrono>
#include <memory>
#include <format>
#include <string>

#define _USE_MATH_DEFINES
#include <math.h>

#include "Utils/Utils.h"

#if FRAMEWORK_UNREAL
#include "Interfaces/Unreal.h"
#endif

#define IMGUI_DEFINE_MATH_OPERATORS
#pragma warning(disable : 4244)
#include "ImGUI/imgui.h"
#include "ImGUI/imgui_impl_win32.h"

#if FRAMEWORK_RENDER_D3D11
#include "ImGUI/imgui_impl_dx11.h"
#endif

#if FRAMEWORK_RENDER_D3D12
#include "ImGUI/imgui_impl_dx12.h"
#endif

#include "ImGUI/Styles.h"

#include "GUI/Custom.h"
#include "GUI/GUI.h"

#define DEG2RAD(deg) deg * M_PI / 180
#define RAD2DEG(rad) rad * 180.0 / M_PI;

namespace Cheat
{
	static const std::string Framework = FRAMEWORK_CODENAME;
	static const std::string Game = FRAMEWORK_TARGET_GAME;

	static const std::string Title = Framework + " (" + Game + ")";

	static bool bShouldRun = true;
	static DWORD dwThreadID = NULL;
	static HMODULE hModule = NULL;

	static constexpr DWORD dwMenuKey = VK_INSERT;
	static constexpr DWORD dwUnloadKey = VK_END;
	static constexpr DWORD dwConsoleKey = VK_HOME;

	inline std::unique_ptr<Console> console = std::make_unique<Console>(false, Title);

	#if FRAMEWORK_UNREAL
	inline std::unique_ptr<Unreal> unreal = std::make_unique<Unreal>();
	#endif
}

#include "Features/Feature.h"

// https://stackoverflow.com/questions/13048301/pointer-to-array-of-base-class-populate-with-derived-class
inline std::vector<std::unique_ptr<Feature>> Features;

#endif //PCH_H