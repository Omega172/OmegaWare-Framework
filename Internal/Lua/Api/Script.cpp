#include "pch.h"
#include "api.h"

namespace Lua {
    std::unique_ptr<Script> Script::New(Manager* pManager, std::variant<std::filesystem::path, std::string> code)
    {
        assert(pManager != nullptr);

        auto script = std::make_unique<Script>();
        script->m_pManager = pManager;
        script->m_code = code;
        script->m_iHash = CRC64::hash(std::holds_alternative<std::filesystem::path>(code) ?
            std::get<std::filesystem::path>(code).string() : std::get<std::string>(code));

        return std::move(script);
    }

    bool Script::Load()
    {
        auto L = m_pManager->m_pState;

        // Load either code from a file or a string.
        if (std::holds_alternative<std::filesystem::path>(m_code)) {
            if (luaL_loadfile(L, std::get<std::filesystem::path>(m_code).string().c_str()) != LUA_OK)
                return false;
        }
        else {
            if (luaL_loadstring(L, std::get<std::string>(m_code).c_str()) != LUA_OK)
                return false;
        }

        // Create env and meta table.
        lua_newtable(L);
        lua_newtable(L);

        // Set the global as the metatable.
        lua_getglobal(L, "_G");
        lua_setfield(L, -2, "__index");
        lua_setmetatable(L, -2);

        // Generate a free unique, should maybe use a secure random here.
        m_sUnique = std::to_string(Utils::Random<uint64_t>());
        lua_getfield(L, LUA_REGISTRYINDEX, m_sUnique.c_str());
        while (!lua_isnil(L, -1))
        {
            lua_pop(L, 1);
            m_sUnique = std::to_string(Utils::Random<uint64_t>());
            lua_getfield(L, LUA_REGISTRYINDEX, m_sUnique.c_str());
        }
        lua_pop(L, 1);

        // Push a new table with the unique as the key to the registry table and set env.
        lua_setfield(L, LUA_REGISTRYINDEX, m_sUnique.c_str());
        lua_getfield(L, LUA_REGISTRYINDEX, m_sUnique.c_str());
        lua_setupvalue(L, 1, 1);

        if (lua_pcall(L, 0, LUA_MULTRET, 0) == LUA_OK)
            return true;

        Unload();
        return false;
    }

    void Script::Unload()
    {
        if (!IsLoaded())
            return;

        auto L = m_pManager->m_pState;

        lua_pushstring(L, m_sUnique.c_str());
        lua_pushnil(L);
        lua_settable(L, LUA_REGISTRYINDEX);
        // reg[m_sUnique] = nil;

        m_sUnique = "";
    }
}