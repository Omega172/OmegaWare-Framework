#pragma once
#if ENGINE_UNREAL

#include "SDK.h"

#define SDK_IMPORTED 0
static_assert(SDK_IMPORTED, "Did you remember to copy in the CG SDK?");

// Reminder remember to incude these files in the project
// BasicTypes_Package.cpp
// CoreUObject_Package.cpp
// Engine_Package.cpp

// If using Dumper 7
// Basic.cpp
// CoreUObject_functions.cpp
// Engine_functions.cpp

#include "FNames.h"
#include "Unreal.h"

#endif