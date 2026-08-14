#pragma once
#pragma execution_character_set("utf-8")

// Identity/versioning macros that both Shared and Internal/Proxy code need at compile time.
// Engine selection (ENGINE_UNREAL/UNITY/OTHER) and renderer selection live in Internal's own
// FrameworkConfig.hpp, since Shared has no opinion on how it's being hosted.

#define STRR(X) #X
#define STR(X) STRR(X)

#define FRAMEWORK_MAJOR_VERSION 7
#define FRAMEWORK_MINOR_VERSION 0
#define FRAMEWORK_REWORK_VERSION 0
#define FRAMEWORK_VERSION FRAMEWORK_MAJOR_VERSION.FRAMEWORK_MINOR_VERSION.FRAMEWORK_REWORK_VERSION

#define FRAMEWORK_CODENAME "OmegaWare"
// Placeholder so the framework builds out of the box. Set this to the game you're targeting.
#define TARGET_GAME_NAME "ExampleGame"

#pragma warning( push ) // disable "operator '!=': deprecated for array types" warning
#pragma warning( disable : 5056)
static_assert(TARGET_GAME_NAME != "", "Target game not set, this HAS to be set or it fucks up the logging system, the console, the menu, and the config system.");
#pragma warning( pop )
