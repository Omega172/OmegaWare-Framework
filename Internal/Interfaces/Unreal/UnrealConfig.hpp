#pragma once
#if ENGINE_UNREAL

#pragma warning(push)
#pragma warning(disable : 4369)

#define SDK_IMPORTED 1
static_assert(SDK_IMPORTED, "Did you remember to copy in the SDK?");

// Reminder remember to incude these files in the project (If using Dumper-7)
// Basic.cpp
// CoreUObject_functions.cpp
// Engine_functions.cpp

#include "SDK.hpp"

#include "Unreal.hpp"

#pragma warning(pop)

#endif