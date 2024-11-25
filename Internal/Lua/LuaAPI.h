#pragma once
#include "Headers/lua.hpp"


class Lua
{
private:
	lua_State* luaState;

public:
	Lua()
	{
		luaState = luaL_newstate();
		luaL_openlibs(luaState);
	}

	~Lua()
	{
		lua_close(luaState);
	}

	void ExecuteScript(std::string sScript)
	{
		int iResult = luaL_dostring(luaState, sScript.c_str());
		if (iResult != LUA_OK)
			LogErrorHere(lua_tostring(luaState, -1));
	}
};