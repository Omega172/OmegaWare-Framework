#pragma once
#include "Headers/lua.hpp"

static lua_State* L;

void InitLUA()
{
	L = luaL_newstate();
	luaL_openlibs(L);
}