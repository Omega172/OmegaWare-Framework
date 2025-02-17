/*
     ██████╗ ███╗   ███╗███████╗ ██████╗  █████╗ ██╗    ██╗ █████╗ ██████╗ ███████╗
    ██╔═══██╗████╗ ████║██╔════╝██╔════╝ ██╔══██╗██║    ██║██╔══██╗██╔══██╗██╔════╝
    ██║   ██║██╔████╔██║█████╗  ██║  ███╗███████║██║ █╗ ██║███████║██████╔╝█████╗
    ██║   ██║██║╚██╔╝██║██╔══╝  ██║   ██║██╔══██║██║███╗██║██╔══██║██╔══██╗██╔══╝
    ╚██████╔╝██║ ╚═╝ ██║███████╗╚██████╔╝██║  ██║╚███╔███╔╝██║  ██║██║  ██║███████╗
     ╚═════╝ ╚═╝     ╚═╝╚══════╝ ╚═════╝ ╚═╝  ╚═╝ ╚══╝╚══╝ ╚═╝  ╚═╝╚═╝  ╚═╝╚══════╝

    This header file defines the core configuration macros and settings for the OmegaWare
    framework. It ensures that essential settings such as framework versioning, engine 
    selection, and rendering API choices are properly set before compilation.

    -------------------------------------------------------------------------------------
    Versioning:
    - FRAMEWORK_MAJOR_VERSION, FRAMEWORK_MINOR_VERSION, FRAMEWORK_REWORK_VERSION define
      the versioning scheme of the framework.
    - FRAMEWORK_CODENAME specifies the framework's codename.

    -------------------------------------------------------------------------------------
    Target Game & Engine:
    - TARGET_GAME_NAME must be explicitly defined to avoid runtime issues.
    - ENGINE_UNREAL, ENGINE_UNITY, and ENGINE_OTHER ensure only one engine type is selected.
    
    -------------------------------------------------------------------------------------
    Rendering API Selection:
    - FRAMEWORK_RENDER_DYNAMIC, FRAMEWORK_RENDER_D3D11, and FRAMEWORK_RENDER_D3D12 ensure 
      only one rendering API is enabled at a time.
    
    -------------------------------------------------------------------------------------
    Warnings & Assertions:
    - Static assertions are used to enforce critical configurations (e.g., ensuring only 
      one engine type and one rendering API are enabled).

    -------------------------------------------------------------------------------------
    Included Dependencies:
    - Standard framework utilities, ImGui library, custom GUI components, and engine-specific 
      configuration files.
    - D3D11/D3D12 libraries and dependencies are conditionally included based on rendering API selection.
    
    -------------------------------------------------------------------------------------
    Notes:
    - Do not modify or remove the "pch.cpp" file, as it is necessary for proper compilation.
    - Ensure that exactly one engine type and one rendering API are enabled to prevent conflicts.
*/

#pragma once
#pragma execution_character_set("utf-8")

#define STRR(X) #X  
#define STR(X) STRR(X)

#define FRAMEWORK_MAJOR_VERSION 6
#define FRAMEWORK_MINOR_VERSION 9
#define FRAMEWORK_REWORK_VERSION 1
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
#define FRAMEWORK_RENDER_D3D11		1
#define FRAMEWORK_RENDER_D3D12		0
static_assert((FRAMEWORK_RENDER_DYNAMIC + FRAMEWORK_RENDER_D3D11 + FRAMEWORK_RENDER_D3D12) == 1, "Only one rendering API can be selected at a time,");

#define SPOOF_THREAD_ADDRESS 0
#define SPOOF_RETURN_ADDRESSES 0

#include "Utils/Utils.h"

#include "Config/Config.h"
#include "Localization/Localization.h"

#include "Memory/Memory.h"

// Unreal
#if ENGINE_UNREAL
#include "Interfaces/Unreal/UnrealConfig.h"
#endif

// Unity
#if ENGINE_UNITY
#include "Interfaces/Unity/UnityConfig.h"
#endif

// Include the ImGui implementation for the rendering API that is being used
#if FRAMEWORK_RENDER_D3D11 || FRAMEWORK_RENDER_DYNAMIC
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#include <d3d11.h>
#include <dxgi1_2.h>
#include "Libs/ImGui/backends/imgui_impl_dx11.h"
#endif

#if FRAMEWORK_RENDER_D3D12 || FRAMEWORK_RENDER_DYNAMIC
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#include <D3D12.h>
#include <dxgi1_4.h>
#include "Libs/ImGui/backends/imgui_impl_dx12.h"
#endif

#include "GUI/Styles.h" // Custom ImGUI colors and font
#include "GUI/Custom.h" // Contains the custom ImGui widgets for the framework
#include "GUI/GUI.h"	// Contains the GUI class that is used to create the all of the framework's menu elements

#include "Hooks/WndProc/WndProcHooks.h"
#include "Hooks/Renderer/RendererHooks.h"