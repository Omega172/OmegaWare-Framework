#pragma once

// Only include headers that rarely change here; keeps rebuild times down across the whole
// Shared target the same way the original single-target pch.h did.

#define NOMINMAX
#include <Windows.h>
#include <shlobj_core.h>

#include <iostream>
#include <cstdio>

#include <string>
#include <sstream>
#include <fstream>
#include <format>

#include <future>
#include <thread>
#include <chrono>

#include <memory>
#include <mutex>

#include <functional>
#include <algorithm>

#include <span>
#include <vector>
#include <unordered_map>

#include <random>

#define _USE_MATH_DEFINES
#include <math.h>
#include <cmath>

#include <filesystem>
#include <optional>
#include <source_location>
#include <stdexcept>
#include <cassert>

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_stdlib.h>
#include <imgui_freetype.h>
#include "../GUI/Addons/imgui_addons.hpp"

#include "../GUI/Fonts/IconsFontAwesome6.hpp"
#include "../GUI/Fonts/IconsFontAwesome6Brands.hpp"
#include "../GUI/Fonts/MuseoSans.hpp"
#include "../GUI/Fonts/Poppins_Medium.hpp"

#include <nlohmann/json.hpp>

#include "../Libs/CRC64/CRC64.hpp"

#define STB_OMIT_TESTS
#include "../Libs/StringToBytes/stb.hpp"

#include "../FrameworkConfig.hpp"

#include "../GUI/Styles.hpp"
#include "../GUI/Custom.hpp"

#include "../Localization/Localization.hpp"
#include "../GUI/Widgets/Elements.hpp"

#include "../Features/Feature.hpp"

#include "../Memory/PointerSafety.hpp"
#include "../Hooks/HookStatus.hpp"
#include "../Utils/Logging/Colors.hpp"
#include "../Utils/Logging/Logging.hpp"
#include "../Utils/Utils.hpp"

#include "../Config/Config.hpp"
