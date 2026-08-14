/*
     ██████╗ ███╗   ███╗███████╗ ██████╗  █████╗ ██╗    ██╗ █████╗ ██████╗ ███████╗
    ██╔═══██╗████╗ ████║██╔════╝██╔════╝ ██╔══██╗██║    ██║██╔══██╗██╔══██╗██╔════╝
    ██║   ██║██╔████╔██║█████╗  ██║  ███╗███████║██║ █╗ ██║███████║██████╔╝█████╗
    ██║   ██║██║╚██╔╝██║██╔══╝  ██║   ██║██╔══██║██║███╗██║██╔══██║██╔══██╗██╔══╝
    ╚██████╔╝██║ ╚═╝ ██║███████╗╚██████╔╝██║  ██║╚███╔███╔╝██║  ██║██║  ██║███████╗
     ╚═════╝ ╚═╝     ╚═╝╚══════╝ ╚═════╝ ╚═╝  ╚═╝ ╚══╝╚══╝ ╚═╝  ╚═╝╚═╝  ╚═╝╚══════╝

    Engine and renderer selection for the Internal target. Identity/version macros live in
    Shared/FrameworkConfig.hpp since Proxy needs them too; this file adds the parts that only
    make sense once you're actually loaded into a target process.
*/

#pragma once

#include "../Shared/FrameworkConfig.hpp"

#define ENGINE_UNREAL	0
#define ENGINE_UNITY	0
#define ENGINE_OTHER	1 // Other is just the catch all for any other game engine or project

static_assert((ENGINE_UNREAL + ENGINE_UNITY + ENGINE_OTHER) == 1, "Only one target engine type can be selected at a time."); // Don't allow both frameworks to be used

// Make sure a rendering API is selected and only one rendering API is selected
#define FRAMEWORK_RENDER_DYNAMIC	1
#define FRAMEWORK_RENDER_D3D11		0
#define FRAMEWORK_RENDER_D3D12		0
static_assert((FRAMEWORK_RENDER_DYNAMIC + FRAMEWORK_RENDER_D3D11 + FRAMEWORK_RENDER_D3D12) == 1, "Only one rendering API can be selected at a time,");

#include "Memory/Memory.hpp"

// Unreal
#if ENGINE_UNREAL
#include "Interfaces/Unreal/UnrealConfig.hpp"
#endif

// Unity
#if ENGINE_UNITY
#include "Interfaces/Unity/UnityConfig.hpp"
#endif

// Include the ImGui implementation for the rendering API that is being used
#if FRAMEWORK_RENDER_D3D11 || FRAMEWORK_RENDER_DYNAMIC
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#include <d3d11.h>
#include <dxgi1_2.h>
#include <imgui_impl_dx11.h>
#endif

#if FRAMEWORK_RENDER_D3D12 || FRAMEWORK_RENDER_DYNAMIC
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#include <D3D12.h>
#include <dxgi1_4.h>
#include <imgui_impl_dx12.h>
#endif

#include <imgui_impl_win32.h>

#include "GUI/GUI.hpp" // The framework's specific menu instance; built from the Shared widget library

#include "Hooks/WndProc/WndProcHooks.hpp"
#include "Hooks/Renderer/RendererHooks.hpp"
