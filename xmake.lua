add_rules("mode.debug", "mode.release")

if is_mode("debug") then
    set_symbols("debug", "edit")
end

option("avx2")
    set_default(true)
    set_showmenu(true)
    set_description("Enable AVX2 optimizations")
option_end()

option("unreal")
    set_default(true)
    set_showmenu(true)
    set_description("Enable compilation of the Unreal target interface (header-only until an SDK dump is added; gates future SDK .cpp files)")
option_end()

option("unity")
    set_default(true)
    set_showmenu(true)
    set_description("Enable compilation of the Unity target interface (header-only until an SDK dump is added; gates future SDK .cpp files)")
option_end()

option("proxy_dll_name")
    set_default("version.dll")
    set_showmenu(true)
    set_description("Filename the Proxy target builds as, so it loads in place of a real system DLL the target game already loads (e.g. version.dll, winmm.dll, dinput8.dll)")
option_end()

set_runtimes(is_mode("debug") and "MTd" or "MT")

-- ImGui is vendored as source under Source/Shared/Vendor/ImGui and Source/Internal/Vendor/ImGui
-- (core + FreeType font builder in Shared, win32/D3D11/D3D12 backends in Internal) rather than
-- pulled from vcpkg. vcpkg's imgui port needs non-default features (win32/dx11/dx12/freetype
-- bindings), and xmake's vcpkg integration has a bug verifying feature-qualified package
-- installs in manifest mode (confirmed against xmake v3.0.8 and v3.0.9): it reports the package
-- as not found even immediately after vcpkg itself reports a fully successful install. Vendoring
-- is also just normal practice for ImGui - most consumers build it as part of their own project
-- rather than as a prebuilt package.
add_requires("vcpkg::freetype", {configs = {debug = is_mode("debug")}})
add_requires("vcpkg::nlohmann-json", {configs = {debug = is_mode("debug")}})
add_requires("vcpkg::polyhook2", {configs = {debug = is_mode("debug")}})

local frameworkPackages = {"vcpkg::freetype", "vcpkg::nlohmann-json", "vcpkg::polyhook2"}

-- Common building block for Internal/Proxy. Depended on, never depends back: no D3D headers,
-- no process-attach state, no game-process-specific memory access.
target("Shared")
    if has_config("avx2") then
        add_vectorexts("avx2")
    end

    set_languages("c++latest")
    set_kind("static")
    set_targetdir(is_mode("debug") and "Build/Debug" or "Build/Release")
    set_pcxxheader("Source/Shared/PCH/pch.h")

    add_includedirs("Source/Shared", "Source/Shared/PCH", { public = true })
    add_includedirs(
        "Source/Shared/Vendor/ImGui",
        "Source/Shared/Vendor/ImGui/misc/cpp",
        "Source/Shared/Vendor/ImGui/misc/freetype",
        { public = true })
    add_defines("IMGUI_ENABLE_FREETYPE", { public = true })

    add_files("Source/Shared/**.cpp")

    add_packages(table.unpack(frameworkPackages), { public = true })

target_end()

-- In-process target: loaded into the game/host process, owns rendering hooks and the
-- actual running framework instance (console, config, menu, features).
target("Internal")
    if has_config("avx2") then
        add_vectorexts("avx2")
    end

    set_languages("c++latest")
    set_kind("shared")
    set_targetdir(is_mode("debug") and "Build/Debug/Internal" or "Build/Release/Internal")
    set_pcxxheader("Source/Internal/PCH/pch.h")

    add_deps("Shared")
    add_includedirs("Source/Internal", "Source/Internal/PCH")
    add_includedirs("Source/Internal/Vendor/ImGui/backends")

    add_files("Source/Internal/**.cpp")

    -- Once an Unreal/Unity SDK dump is added, its generated .cpp files get added here, gated
    -- by the same options that already control the header-only interfaces:
    if has_config("unreal") then add_files("Source/Internal/Interfaces/Unreal/**.cpp") end
    if has_config("unity") then add_files("Source/Internal/Interfaces/Unity/**.cpp") end

    add_packages(table.unpack(frameworkPackages))
    add_syslinks("d3d11", "d3d12", "dxgi")

target_end()

-- Same framework as Internal, built a second time under PROXY and renamed to a system DLL the
-- target game already loads on startup (see proxy_dll_name), so it gets loaded without a
-- separate injector. ProxyMain.cpp resolves and forwards that DLL's real exports (listed in
-- Proxy.def) from the genuine system copy at runtime via LoadLibrary/GetProcAddress - see
-- Source/Internal/dllmain.cpp's #ifdef PROXY block for where that gets initialized.
target("Proxy")
    if has_config("avx2") then
        add_vectorexts("avx2")
    end

    set_languages("c++latest")
    set_kind("shared")
    set_targetdir(is_mode("debug") and "Build/Debug/Proxy" or "Build/Release/Proxy")
    set_filename(get_config("proxy_dll_name"))
    set_pcxxheader("Source/Internal/PCH/pch.h")

    add_deps("Shared")
    add_includedirs("Source/Internal", "Source/Internal/PCH")
    add_includedirs("Source/Internal/Vendor/ImGui/backends")
    add_defines("PROXY")

    add_files("Source/Internal/**.cpp")
    add_files("Source/Proxy/*.cpp")
    add_files("Source/Proxy/Proxy.def")

    if has_config("unreal") then add_files("Source/Internal/Interfaces/Unreal/SDK/**.cpp") end
    if has_config("unity") then add_files("Source/Internal/Interfaces/Unity/SDK/**.cpp") end

    add_packages(table.unpack(frameworkPackages))
    add_syslinks("d3d11", "d3d12", "dxgi")

target_end()
