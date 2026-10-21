local stDefaults = {
    strName = "OmegaWare",
    strVersion = "4.0.0",
    strGame = "ExampleGame",
    strProcess = "",
    bGameDepth = true,
}

set_config("builddir", "Build")

set_project("OmegaWare")
set_version(get_config("framework_version") or stDefaults.strVersion)

add_rules("mode.debug", "mode.release")

if is_mode("debug") then
    set_symbols("debug", "edit")
end

local function LibraryDefault(strName, strLibrary)
    for _, strPath in ipairs({ path.join(os.scriptdir(), "Source", "Third-Party", "OmegaWareProjects", strName), path.join(os.scriptdir(), "..", strName) }) do
        if os.isfile(path.join(strPath, "Source", strLibrary, "xmake.lua")) then
            return strPath
        end
    end
    return ""
end

option("engine")
    set_default("none")
    set_showmenu(true)
    set_values("none", "unreal", "unity")
    set_description("Game engine the OmegaWare target is built for (unreal needs a Dumper-7 SDK, see unreal_sdk)")
option_end()

option("framework_name")
    set_default(stDefaults.strName)
    set_showmenu(true)
    set_description("Framework name: DLL file name, data folder under Documents, crash dumps and DLL version info (lowercase in the menu, log and console)")
option_end()

option("framework_version")
    set_default(stDefaults.strVersion)
    set_showmenu(true)
    set_description("Framework version as major.minor.patch, shown in the menu and log and written to the DLL version info")
option_end()

option("game")
    set_default(stDefaults.strGame)
    set_showmenu(true)
    set_description("Name of the game this build targets, shown in the menu, log and DLL version info")
option_end()

option("process")
    set_default(stDefaults.strProcess)
    set_showmenu(true)
    set_description("Executable the framework may start in, for example Palworld-Win64-Shipping.exe (empty starts in any process)")
option_end()

option("unreal_sdk")
    set_default(path.join(os.scriptdir(), "Source", "Third-Party", "Dumper-7", "CppSDK"))
    set_showmenu(true)
    set_description("Path to a Dumper-7 CppSDK folder (the one containing SDK.hpp) for engine=unreal")
option_end()

option("game_depth")
    set_default(stDefaults.bGameDepth)
    set_showmenu(true)
    set_description("Track the game's depth buffer so world drawing is hidden behind geometry (off avoids hooking the game's DX11/DX12 command lists)")
option_end()

option("proxy")
    set_default("version")
    set_showmenu(true)
    set_values("version", "winmm", "dinput8")
    set_description("System DLL the OmegaWareProxy target replaces (exports come from Source/OmegaWareProxy/Exports, regenerate with GenerateProxyExports)")
option_end()

option("gui")
    set_default(LibraryDefault("OmegaWare-GUI", "OmegaGUI"))
    set_showmenu(true)
    set_description("Path to OmegaWare-GUI for the OmegaWare target (empty to disable)")
option_end()

option("renderhook")
    set_default(LibraryDefault("OmegaWare-RenderHook", "OmegaRenderHook"))
    set_showmenu(true)
    set_description("Path to OmegaWare-RenderHook for the OmegaWare target (empty to disable)")
option_end()

option("localization")
    set_default(LibraryDefault("OmegaWare-Localization", "OmegaLocalization"))
    set_showmenu(true)
    set_description("Path to OmegaWare-Localization for the OmegaWare target (empty to disable)")
option_end()

set_runtimes(is_mode("debug") and "MTd" or "MT")

add_requires("vcpkg::nlohmann-json", {configs = {debug = is_mode("debug")}})

local aOmegaLibraries = {
    { strOption = "gui", strFile = path.join("Source", "OmegaGUI", "xmake.lua") },
    { strOption = "renderhook", strFile = path.join("Source", "OmegaRenderHook", "xmake.lua") },
    { strOption = "localization", strFile = path.join("Source", "OmegaLocalization", "xmake.lua") },
}

local bOmegaLibraries = true
for _, stLibrary in ipairs(aOmegaLibraries) do
    local strRoot = get_config(stLibrary.strOption)
    if not strRoot or strRoot == "" or not os.isfile(path.join(strRoot, stLibrary.strFile)) then
        bOmegaLibraries = false
    end
end

if not bOmegaLibraries then
    target("OmegaWare")
        set_kind("phony")
        on_config(function (target)
            raise("OmegaWare needs OmegaWare-GUI, OmegaWare-RenderHook and OmegaWare-Localization: run git submodule update --init, or pass their paths in --gui, --renderhook and --localization")
        end)
    target_end()
else
    for _, stLibrary in ipairs(aOmegaLibraries) do
        includes(path.join(get_config(stLibrary.strOption), stLibrary.strFile))
    end

    local function ConfigureOmegaWare()
        set_kind("shared")
        set_languages("c++latest")
        set_encodings("utf-8")
        add_defines("NOMINMAX", "WIN32_LEAN_AND_MEAN", "UNICODE", "_UNICODE")

        add_deps("OmegaGUI", "OmegaRenderHook", "OmegaLocalization")
        add_packages("vcpkg::nlohmann-json")
        add_includedirs("Source/OmegaWare", "Source/OmegaWarePlugin")
        add_defines("OMEGAWARE_PLUGIN_HOST=1")
        add_defines("OMEGAWARE_GAME_DEPTH=" .. (has_config("game_depth") and "1" or "0"))
        add_files("Source/OmegaWare/**.cpp|Engines/Unity/**.cpp|Engines/Unreal/**.cpp")

        local strConfigDir = path.join(os.scriptdir(), "Build", "Config", "$(arch)", "$(mode)")
        set_configdir(strConfigDir)
        local strName = get_config("framework_name") or stDefaults.strName
        set_configvar("OMEGAWARE_NAME", strName)
        set_configvar("OMEGAWARE_TITLE", strName:lower())
        set_configvar("OMEGAWARE_GAME", get_config("game") or stDefaults.strGame)
        set_configvar("OMEGAWARE_PROCESS", get_config("process") or stDefaults.strProcess)
        add_configfiles("Source/OmegaWare/Core/Version.hpp.in")
        add_includedirs(strConfigDir)
        add_files("Source/OmegaWare/Version.rc")

        local strEngine = get_config("engine") or "none"
        if strEngine == "unity" then
            add_defines("OMEGAWARE_ENGINE_UNITY=1")
            add_files("Source/OmegaWare/Engines/Unity/*.cpp")
        elseif strEngine == "unreal" then
            local strSdk = get_config("unreal_sdk") or ""
            on_config(function (target)
                local strConfiguredSdk = get_config("unreal_sdk") or ""
                local aMissing = {}
                for _, strFile in ipairs({ "SDK.hpp", "Assertions.inl", "NameCollisions.inl" }) do
                    if not os.isfile(path.join(strConfiguredSdk, strFile)) then
                        table.insert(aMissing, strFile)
                    end
                end
                if #aMissing > 0 then
                    raise("engine=unreal needs a Dumper-7 SDK in " .. strConfiguredSdk .. " (missing " .. table.concat(aMissing, ", ") .. "): copy the CppSDK folder Dumper-7 generated there or set --unreal_sdk=<path>")
                end
                local strOutDir = path.join(os.projectdir(), "Build", "Config", get_config("arch"), get_config("mode"))
                local strTable = path.join(strOutDir, "SdkAudit.txt")
                local strStamp = path.join(strOutDir, "SdkAudit.stamp")
                local strResource = path.join(strOutDir, "SdkAudit.rc")
                local aFiles = {}
                for _, strSuffix in ipairs({ "_classes.hpp", "_structs.hpp", "_parameters.hpp" }) do
                    table.join2(aFiles, os.files(path.join(strConfiguredSdk, "SDK", "*" .. strSuffix)))
                end
                table.sort(aFiles)
                local iTime = 0
                for _, strFile in ipairs(aFiles) do
                    iTime = math.max(iTime, os.mtime(strFile))
                end
                local strCurrent = "v3 " .. path.absolute(strConfiguredSdk) .. " " .. #aFiles .. " " .. iTime
                if not os.isfile(strTable) or not os.isfile(strStamp) or io.readfile(strStamp) ~= strCurrent then
                    local aLines = {}
                    local iMembers = 0
                    for _, strFile in ipairs(aFiles) do
                        local strOwner = nil
                        local bWritten = false
                        for strLine in io.readfile(strFile):gmatch("[^\r\n]+") do
                            local strKind, strName = strLine:match("^// (%a+) (.-)%s*$")
                            if strLine:find("^};") then
                                strOwner = nil
                            elseif strKind then
                                bWritten = false
                                if strKind == "Function" then
                                    strOwner = "F " .. strName
                                elseif strKind == "ScriptStruct" then
                                    strOwner = "S " .. strName
                                elseif strKind:find("Class$") then
                                    strOwner = "C " .. strName
                                else
                                    strOwner = nil
                                end
                            elseif strOwner then
                                local strLeft, strOffset = strLine:match("^%s+([^/]-);%s*// 0x(%x+)%(0x%x+%)")
                                if strLeft and not strLine:find("NOT AUTO-GENERATED", 1, true) then
                                    strLeft = strLeft:gsub("%b[]", ""):gsub(":%s*%d+%s*$", "")
                                    local strMember = strLeft:match("([%w_]+)%s*$")
                                    if strMember and not strMember:find("^Pad_") and not strMember:find("^BitPad_") then
                                        if not bWritten then
                                            table.insert(aLines, strOwner)
                                            bWritten = true
                                        end
                                        table.insert(aLines, strMember .. " " .. strOffset)
                                        iMembers = iMembers + 1
                                    end
                                end
                            end
                        end
                    end
                    os.mkdir(strOutDir)
                    io.writefile(strTable, table.concat(aLines, "\n") .. "\n")
                    io.writefile(strStamp, strCurrent)
                    cprint("${dim}SDK audit table: %d members from %d headers", iMembers, #aFiles)
                end
                io.writefile(strResource, "OMEGAWARE_SDK_AUDIT RCDATA \"" .. (strTable:gsub("\\", "\\\\")) .. "\"\n")
                target:add("files", strResource)
            end)
            add_defines("OMEGAWARE_ENGINE_UNREAL=1")
            add_files("Source/OmegaWare/Engines/Unreal/*.cpp")
            add_includedirs(strSdk)
            for _, strFile in ipairs({ "Basic.cpp", "CoreUObject_functions.cpp", "Engine_functions.cpp" }) do
                if os.isfile(path.join(strSdk, "SDK", strFile)) then
                    add_files(path.join(strSdk, "SDK", strFile), { cxflags = { "/wd4005", "/wd4369" } })
                end
            end
        end
    end

    target("OmegaWare")
        ConfigureOmegaWare()
        set_targetdir(path.join("Build", is_mode("debug") and "Debug" or "Release", "$(arch)", "OmegaWare"))
        set_filename((get_config("framework_name") or stDefaults.strName) .. ".dll")
    target_end()

    local function ConfigureProxy(strProxy, strFolder)
        ConfigureOmegaWare()
        set_targetdir(path.join("Build", is_mode("debug") and "Debug" or "Release", "$(arch)", strFolder))
        set_filename(strProxy .. ".dll")
        add_defines("OMEGAWARE_PROXY=1", "OMEGAWARE_PROXY_DLL=\"" .. strProxy .. ".dll\"")
        add_files(path.join("Source", "OmegaWareProxy", "Exports", strProxy .. ".cpp"))
    end

    target("OmegaWareProxy")
        ConfigureProxy(get_config("proxy"), "OmegaWareProxy")
    target_end()

    for _, strProxy in ipairs({ "version", "winmm", "dinput8" }) do
        target("OmegaWareProxy-" .. strProxy)
            ConfigureProxy(strProxy, "OmegaWareProxies")
            set_default(false)
            set_group("Proxies")
        target_end()
    end

    target("ExamplePlugin")
        set_kind("shared")
        set_languages("c++latest")
        set_encodings("utf-8")
        set_targetdir(path.join("Build", is_mode("debug") and "Debug" or "Release", "$(arch)", "ExamplePlugin"))
        add_defines("NOMINMAX", "WIN32_LEAN_AND_MEAN", "UNICODE", "_UNICODE")
        add_includedirs("Source/OmegaWarePlugin")
        add_files("Source/ExamplePlugin/*.cpp")
    target_end()

    for _, strPluginDir in ipairs(os.dirs(path.join(os.scriptdir(), "Source", "Plugins", "*"))) do
        local strPlugin = path.filename(strPluginDir)
        target(strPlugin)
            set_kind("shared")
            set_languages("c++latest")
            set_encodings("utf-8")
            set_group("Plugins")
            set_targetdir(path.join("Build", is_mode("debug") and "Debug" or "Release", "$(arch)", "Plugins"))
            add_defines("NOMINMAX", "WIN32_LEAN_AND_MEAN", "UNICODE", "_UNICODE")
            add_includedirs("Source/OmegaWarePlugin", strPluginDir)
            add_files(path.join(strPluginDir, "**.cpp"))
            if (get_config("engine") or "none") == "unreal" then
                local strSdk = get_config("unreal_sdk") or ""
                add_defines("OMEGAWARE_ENGINE_UNREAL=1")
                add_includedirs(strSdk)
                for _, strFile in ipairs({ "Basic.cpp", "CoreUObject_functions.cpp", "Engine_functions.cpp" }) do
                    if os.isfile(path.join(strSdk, "SDK", strFile)) then
                        add_files(path.join(strSdk, "SDK", strFile), { cxflags = { "/wd4005", "/wd4369" } })
                    end
                end
            end
        target_end()
    end

    target("OmegaWareTests")
        set_kind("binary")
        set_languages("c++latest")
        set_encodings("utf-8")
        set_targetdir(path.join("Build", is_mode("debug") and "Debug" or "Release", "$(arch)", "OmegaWareTests"))
        add_defines("NOMINMAX", "WIN32_LEAN_AND_MEAN", "UNICODE", "_UNICODE")

        add_deps("OmegaRenderHook")
        add_defines("OMEGAWARE_PROXY_DLL=\"" .. get_config("proxy") .. ".dll\"")
        add_syslinks("version")
        add_includedirs("Source/OmegaWare")
        add_files("Source/OmegaWareTests/*.cpp")
        add_files("Source/OmegaWare/Memory/*.cpp", "Source/OmegaWare/Core/Log.cpp", "Source/OmegaWare/Core/Console.cpp", "Source/OmegaWare/Core/CrashDump.cpp", "Source/OmegaWare/Engines/Unreal/NamePool.cpp")
        if (get_config("engine") or "none") == "unity" then
            add_defines("OMEGAWARE_ENGINE_UNITY=1")
            add_files("Source/OmegaWare/Engines/Unity/*.cpp")
        end
    target_end()
end
