add_rules("mode.debug", "mode.release")

add_includedirs("Internal", "Internal/PCH", "Internal/Libs/ImGui", "Internal/Libs/FreeType/include")
set_pcxxheader("Internal/PCH/pch.h")

if is_mode("debug") then
    add_linkdirs("Internal/Libs/MinHook/build/lib/Debug/", "Internal/Libs/FreeType/objs/x64/Debug Static/")
else
    add_linkdirs("Internal/Libs/MinHook/build/lib/Release/", "Internal/Libs/FreeType/objs/x64/Release Static/")
end 

target("Internal")
    add_links(
        "kernel32", "user32", "gdi32", "winspool", "comdlg32",
        "advapi32", "shell32", "ole32", "oleaut32", "uuid",
        "odbc32", "odbccp32"
    )

    if is_mode("debug") then
        set_runtimes("MDd")
        set_targetdir("Build/Debug/Internal")
        add_links("minhook.x64d.lib")

        add_defines("_DEBUG")
        set_strip("none")

        add_cxflags(
            "/permissive-",   -- Enforce standard C++ compliance
            "/GS",            -- Buffer security check
            "/W3",            -- Warning level 3
            "/Zc:wchar_t",    -- Treat wchar_t as a built-in type
            --"/ZI",            -- Edit-and-continue debug information
            "/Od",            -- Disable optimizations for debugging
            "/sdl",           -- Enable additional security checks
            "/Zc:inline",     -- Remove unused inline functions
            "/fp:precise",    -- Floating-point model: precise
            "/WX-",           -- Do not treat warnings as errors
            "/Zc:forScope",   -- Enforce standard C++ for-loop scope
            "/RTC1",          -- Enable runtime error checks (stack frame consistency)
            "/Gd",            -- Use __cdecl calling convention
            "/MDd",           -- Use multi-threaded **debug** DLL runtime
            "/FC",            -- Show full path in diagnostics
            "/EHsc",          -- Enable C++ exception handling
            "/nologo"         -- Suppress startup banner
        )

        add_ldflags(
            "/MANIFEST",       -- Generate a side-by-side manifest
            "/NXCOMPAT",       -- Data Execution Prevention (DEP)
            "/DYNAMICBASE",    -- Enable ASLR (Address Space Layout Randomization)
            "/DEBUG",          -- Generate debugging information
            "/DLL",            -- Build as a DLL
            "/MACHINE:X64",    -- Target x64 architecture
            "/INCREMENTAL",    -- Enable incremental linking
            "/SUBSYSTEM:CONSOLE", -- Console application
            "/MANIFESTUAC:\"level='asInvoker' uiAccess='false'\"", -- UAC manifest settings
            "/NOLOGO",         -- Suppress linker startup banner
            "/TLBID:1"         -- Type library ID
        )
    else
        set_runtimes("MD")
        set_targetdir("Build/Release/Internal")
        add_links("minhook.x64.lib")
        add_undefines("_DEBUG")

        add_cxflags(
            "/permissive-",   -- Enforce standard C++ compliance
            "/GS",            -- Buffer security check
            "/GL",            -- Whole program optimization
            "/W3",            -- Warning level 3
            "/Gy",            -- Function-level linking
            "/Zc:wchar_t",    -- Treat wchar_t as a built-in type
            "/Zi",            -- Generate debug information
            "/O2",            -- Optimize for speed
            "/sdl",           -- Enable additional security checks
            "/Zc:inline",     -- Remove unused inline functions
            "/fp:precise",    -- Floating-point model: precise
            "/WX-",           -- Do not treat warnings as errors
            "/Zc:forScope",   -- Enforce standard C++ for-loop scope
            "/Gd",            -- Use __cdecl calling convention
            "/Oi",            -- Enable intrinsic functions
            "/MD",            -- Use multi-threaded DLL runtime
            "/FC",            -- Show full path in diagnostics
            "/EHsc",          -- Enable C++ exception handling
            "/nologo"         -- Suppress startup banner
        )

        add_ldflags(
            "/MANIFEST",         -- Generate a side-by-side manifest
            "/LTCG:incremental", -- Enable incremental link-time code generation
            "/NXCOMPAT",         -- Enable Data Execution Prevention (DEP)
            "/DYNAMICBASE",      -- Enable ASLR (Address Space Layout Randomization)
            "/DEBUG",            -- Generate debugging information (Release PDBs)
            "/DLL",              -- Build as a DLL
            "/MACHINE:X64",      -- Target x64 architecture
            "/OPT:REF",          -- Remove unreferenced functions/data
            "/OPT:ICF",          -- Perform identical COMDAT folding
            "/SUBSYSTEM:CONSOLE", -- Console application
            "/MANIFESTUAC:\"level='asInvoker' uiAccess='false'\"", -- UAC settings
            "/NOLOGO",           -- Suppress linker startup banner
            "/TLBID:1"           -- Type library ID
        )
    end

    add_links("freetype.lib")

    set_languages("c++latest")
    set_kind("shared")
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

    add_files("Internal/Libs/ImGUI/imgui.cpp")
    add_files("Internal/Libs/ImGUI/imgui_draw.cpp")
    add_files("Internal/Libs/ImGUI/imgui_tables.cpp")
    add_files("Internal/Libs/ImGUI/imgui_widgets.cpp")
    add_files("Internal/Libs/ImGUI/misc/freetype/imgui_freetype.cpp")
    add_files("Internal/Libs/ImGUI/misc/cpp/imgui_stdlib.cpp")
    add_files("Internal/Libs/ImGUI/backends/imgui_impl_dx11.cpp")
    add_files("Internal/Libs/ImGUI/backends/imgui_impl_dx12.cpp")
    add_files("Internal/Libs/ImGUI/backends/imgui_impl_win32.cpp")

    add_files("Internal/Features/*/*.cpp")