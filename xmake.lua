add_rules("mode.debug", "mode.release")

if is_mode("debug") then
    set_symbols("debug", "edit")
end

option("avx2")
    set_default(true)
    set_showmenu(true)
    set_description("Enable AVX2 optimizations")
option_end()

set_targetdir(is_mode("debug") and "Build/Debug" or "Build/Release")
set_runtimes(is_mode("debug") and "MDd" or "MD")

add_requires("vcpkg::minhook", {configs = {debug = is_mode("debug")}})
add_requires("vcpkg::imgui", {configs = {debug = is_mode("debug"), features = {"win32-binding", "dx12-binding", "dx11-binding", "freetype"}}})
add_requires("vcpkg::freetype", {configs = {debug = is_mode("debug")}})
add_requires("vcpkg::nlohmann-json", {configs = {debug = is_mode("debug")}})
add_requires("vcpkg::libpng", {configs = {debug = is_mode("debug")}})
add_requires("vcpkg::brotli", {configs = {debug = is_mode("debug")}})
add_requires("vcpkg::zlib", {configs = {debug = is_mode("debug")}})
add_requires("vcpkg::bzip2", {configs = {debug = is_mode("debug")}})

target("Internal")
    if has_config("avx2") then
        add_vectorexts("avx2")
    end

    set_languages("c++latest")
    set_kind("shared")
    set_pcxxheader("Internal/PCH/pch.h")

    add_includedirs("Internal", "Internal/PCH")

    add_files("Internal/dllmain.cpp")
    add_files("Internal/PCH/pch.cpp")
    add_files("Internal/Utils/Utils.cpp")
    add_files("Internal/Utils/Console/Console.cpp")
    add_files("Internal/Utils/Logging/Logging.cpp")
    add_files("Internal/Memory/Memory.cpp")
    add_files("Internal/Memory/Windows/WindowsMemory.cpp")
    add_files("Internal/Localization/Localization.cpp")
    add_files("Internal/Config/Config.cpp")
    add_files("Internal/Hooks/Renderer/D3D11Hooks.cpp")
    add_files("Internal/Hooks/Renderer/D3D12Hooks.cpp")
    add_files("Internal/Hooks/Renderer/RendererHooks.cpp")
    add_files("Internal/Hooks/WndProc/WndProcHooks.cpp")
    add_files("Internal/GUI/GUI.cpp")
    add_files("Internal/GUI/Addons/imgui_addons.cpp")
    add_files("Internal/Features/*/*.cpp")

    -- If targeting Unreal Engine games, include the SDK files
    --add_files("Internal/Interfaces/Unreal/SDK/Basic.cpp")
    --add_files("Internal/Interfaces/Unreal/SDK/CoreUObject_functions.cpp")
    --add_files("Internal/Interfaces/Unreal/SDK/Engine_functions.cpp")

    add_packages("vcpkg::minhook", "vcpkg::imgui", "vcpkg::freetype", "vcpkg::nlohmann-json", "vcpkg::libpng", "vcpkg::brotli", "vcpkg::zlib", "vcpkg::bzip2")
    add_syslinks("d3d11", "d3d12", "dxgi")