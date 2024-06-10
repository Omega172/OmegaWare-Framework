#pragma once

#pragma execution_character_set("utf-8")

#define FRAMEWORK_MAJOR_VERSION 6
#define FRAMEWORK_MINOR_VERSION 8
#define FRAMEWORK_REWORK_VERSION 7
#define FRAMEWORK_VERSION FRAMEWORK_MAJOR_VERSION.FRAMEWORK_MINOR_VERSION.FRAMEWORK_REWORK_VERSION

#define FRAMEWORK_CODENAME "OmegaWare"
#define FRAMEWORK_TARGET_GAME ""
#define FRAMEWORK_TARGET_PROCESS ""

// Commented these out because I'm not sure they are really necessary anymore and they are pretty annoying
//static_assert(FRAMEWORK_TARGET_GAME != "", "Target game not set.");
//static_assert(FRAMEWORK_TARGET_PROCESS != "", "Target process name not set.");

// Check if x86 or x64
#if defined(_WIN64)
#define FRAMEWORK_X64 1
#else
#define FRAMEWORK_X64 0
#endif

#define FRAMEWORK_OTHER		1 // Other is just the catch all for any other game engine or project
#define FRAMEWORK_UNREAL	0
#define FRAMEWORK_UNITY		0

static_assert((FRAMEWORK_OTHER + FRAMEWORK_UNREAL + FRAMEWORK_UNITY) == 1, "Must use exactly one framework type"); // Don't allow both frameworks to be used)

// Make sure a rendering API is selected and only one rendering API is selected
#define FRAMEWORK_RENDER_DYNAMIC 0
#define FRAMEWORK_RENDER_D3D11 1
#define FRAMEWORK_RENDER_D3D12 0
static_assert((FRAMEWORK_RENDER_DYNAMIC + FRAMEWORK_RENDER_D3D11 + FRAMEWORK_RENDER_D3D12) == 1, "Must use exactly one rendering API");
static_assert(!(FRAMEWORK_RENDER_DYNAMIC || FRAMEWORK_RENDER_D3D12), "This does NOT work right now, please dont use ;3");

#ifndef FRAMEWORK_INJECTOR

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

#include "GUI/Styles.h" // Include the Styles.h file that contains the ImGui styles for the framework

#include "GUI/Custom.h" // Include the Custom.h file that contains the custom ImGui widgets for the framework
#include "GUI/GUI.h" // Include the GUI.h file that contains the GUI class that is used to create the framework's menu

#include "Hooks/WndProc/WndProcHooks.h"
#include "Hooks/Renderer/RendererHooks.h"

#endif