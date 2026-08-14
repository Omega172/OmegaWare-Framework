#pragma once

// Pulls in every Shared header (ImGui, GUI widgets, Localization, Logging, Config, the
// Hook<T>/HookBatch abstraction's dependencies) plus the parts that are only meaningful once
// this DLL is actually loaded into a target process: PolyHook2 itself, the hook abstraction's
// PolyHook2-facing pieces, and the Framework instance defined in Includes.hpp.

#include "../../Shared/PCH/pch.h"

#include <polyhook2/Detour/x64Detour.hpp>
#include <polyhook2/Misc.hpp>
#include "../../Shared/Hooks/Hook.hpp"
#include "../../Shared/Hooks/HookBatch.hpp"

#include "../Includes.hpp"
