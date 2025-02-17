#pragma once

#include "pch.h"
#include "FrameworkConfig.h"

class BaseFeature; //Forward declaration

namespace Framework {
	inline const std::string Framework = FRAMEWORK_CODENAME;
	inline const std::string Game = TARGET_GAME_NAME;
	inline const std::string Title = Framework + " (" + Game + ")";

	inline std::unique_ptr<Console> console = std::make_unique<Console>(false, Title);

	inline bool bInitalized = false;
	inline bool bShouldRun = true;
	inline HMODULE hModule = NULL;
	inline size_t iModuleNameHash = {};

	constexpr ImGuiKey keyMenuKey = ImGuiKey_Insert;
	constexpr ImGuiKey keyUnloadKey1 = ImGuiKey_End; // A DWORD to store the key that unloads the cheat (Cool one)
	constexpr ImGuiKey keyUnloadKey2 = ImGuiKey_Delete; // A DWORD to store the key that unloads the cheat (Lame one)
	constexpr ImGuiKey keyConsoleKey = ImGuiKey_Home;

	inline std::unique_ptr<Config> config;

	inline std::unique_ptr<WndProcHooks> wndproc = std::make_unique<WndProcHooks>();
	inline std::unique_ptr<RendererHooks> renderer = std::make_unique<RendererHooks>();

	inline std::unique_ptr<Menu> menu = std::make_unique<Menu>(Framework::Title, Framework::Title, ElementBase::Style_t({
	.vec2Size = ImVec2{ 600.f, 506.f },
	.iFlags = ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground }));

#if ENGINE_UNREAL
	inline std::unique_ptr<Unreal> unreal = std::make_unique<Unreal>();
#endif

#if ENGINE_UNITY
	inline Mono mono = Mono::Instance();
#endif

	inline std::vector<BaseFeature*> g_vecFeatures{};
}

// Framework Features
#include "../Features/Feature.h"