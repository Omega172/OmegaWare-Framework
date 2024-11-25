#pragma once

#pragma execution_character_set("utf-8")

#define FRAMEWORK_MAJOR_VERSION 6
#define FRAMEWORK_MINOR_VERSION 8
#define FRAMEWORK_REWORK_VERSION 7
#define FRAMEWORK_VERSION FRAMEWORK_MAJOR_VERSION.FRAMEWORK_MINOR_VERSION.FRAMEWORK_REWORK_VERSION

#define FRAMEWORK_CODENAME "OmegaWare"
#define TARGET_GAME_NAME "TEST"

#pragma warning( push ) // disable "operator '!=': deprecated for array types" warning
#pragma warning( disable : 5056)
static_assert(TARGET_GAME_NAME != "", "Target game not set, this HAS to be set or it fucks up the logging system, the console, the menu, and the config system.");
#pragma warning( pop )

#define ENGINE_UNREAL	0
#define ENGINE_UNITY	0
#define ENGINE_OTHER	1 // Other is just the catch all for any other game engine or project

static_assert((ENGINE_UNREAL + ENGINE_UNITY + ENGINE_OTHER) == 1, "Only one target engine type can be selected at a time."); // Don't allow both frameworks to be used)

// Make sure a rendering API is selected and only one rendering API is selected
#define FRAMEWORK_RENDER_DYNAMIC	0
#define FRAMEWORK_RENDER_D3D11		0
#define FRAMEWORK_RENDER_D3D12		1
static_assert((FRAMEWORK_RENDER_DYNAMIC + FRAMEWORK_RENDER_D3D11 + FRAMEWORK_RENDER_D3D12) == 1, "Only one rendering API can be selected at a time,");

#ifndef FRAMEWORK_INJECTOR
#include "Utils/Utils.h"

// Unreal
#if FRAMEWORK_UNREAL
#include "Interfaces/Unreal/UnrealConfig.h"
#endif

// Unity
#if FRAMEWORK_UNITY
#include "Interfaces/Unity/UnityConfig.h"
#endif

#define IMGUI_DEFINE_MATH_OPERATORS
#include "Libs/ImGui/imgui.h"
#include "Libs/ImGui/imgui_impl_win32.h"

// Include the ImGui implementation for the rendering API that is being used
#if FRAMEWORK_RENDER_D3D11 || FRAMEWORK_RENDER_DYNAMIC
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#include <d3d11.h>
#include <dxgi1_2.h>
#include "Libs/ImGui/imgui_impl_dx11.h"
#endif

#if FRAMEWORK_RENDER_D3D12 || FRAMEWORK_RENDER_DYNAMIC
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#include <D3D12.h>
#include <dxgi1_4.h>
#include "Libs/ImGui/imgui_impl_dx12.h"
#endif

#include "GUI/Styles.h" // Custom ImGUI colors and font
#include "GUI/Custom.h" // Contains the custom ImGui widgets for the framework
#include "GUI/GUI.h"	// Contains the GUI class that is used to create the all of the framework's menu elements

#include "Hooks/WndProc/WndProcHooks.h"
#include "Hooks/Renderer/RendererHooks.h"

#endif