/*
     ██████╗ ███╗   ███╗███████╗ ██████╗  █████╗ ██╗    ██╗ █████╗ ██████╗ ███████╗
    ██╔═══██╗████╗ ████║██╔════╝██╔════╝ ██╔══██╗██║    ██║██╔══██╗██╔══██╗██╔════╝
    ██║   ██║██╔████╔██║█████╗  ██║  ███╗███████║██║ █╗ ██║███████║██████╔╝█████╗  
    ██║   ██║██║╚██╔╝██║██╔══╝  ██║   ██║██╔══██║██║███╗██║██╔══██║██╔══██╗██╔══╝  
    ╚██████╔╝██║ ╚═╝ ██║███████╗╚██████╔╝██║  ██║╚███╔███╔╝██║  ██║██║  ██║███████╗
     ╚═════╝ ╚═╝     ╚═╝╚══════╝ ╚═════╝ ╚═╝  ╚═╝ ╚══╝╚══╝ ╚═╝  ╚═╝╚═╝  ╚═╝╚══════╝

    Note for pch.h:
    - Only include headers that rarely change.
    - This ensures faster compilation times by minimizing rebuilds.
    - Examples: Standard library headers, third-party library headers, etc.

    Avoid headers that change frequently, such as:
    - Project-specific headers (unless stable).
    - Headers under active development.
*/

#pragma once

#define NOMINMAX
#include <Windows.h>

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

#include <random>

#define _USE_MATH_DEFINES // Define math constants for things like M_PI and M_SQRT2
#include <math.h>
#include <cmath>

#include <filesystem>

#include <optional>

#include <source_location>

#include <shlobj_core.h>

#include <future>

#include <stdexcept>

#include <cassert>

#include "../Libs/MinHook/include/MinHook.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "../Libs/ImGUI/imgui.h"
#include "../Libs/ImGui/backends/imgui_impl_win32.h"

#include "../Libs/Nlohmann.json/single_include/nlohmann/json.hpp"

#include "../Libs/CRC64/CRC64.h"

#define STB_OMIT_TESTS
#include "../Libs/StringToBytes/stb.h"