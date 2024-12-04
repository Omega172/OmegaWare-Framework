#pragma once
#include "Headers/lua.hpp"


struct LuaFile_T
{
	bool selected = false;
	bool loaded = false;
	std::filesystem::path file;
};

class Lua
{
private:
	lua_State* luaState;

public:
	LuaFile_T stSelectedFile;
	std::vector<LuaFile_T> vecLoadedFiles;

	Lua()
	{
		Utils::LogDebug("Initializing Lua");
		luaState = luaL_newstate();
		luaL_openlibs(luaState);
	}

	~Lua()
	{
		Utils::LogDebug("Destroying Lua");
		lua_close(luaState);
	}

	std::vector<LuaFile_T> GetLuaFiles()
	{
		std::vector<LuaFile_T> vecLuaFiles;
		
		std::optional<std::filesystem::path> pathLua = Utils::GetLuaFilePath();
		if (!pathLua)
			return vecLuaFiles;

		for (const auto& entry : std::filesystem::directory_iterator(pathLua.value()))
		{
			if (entry.is_regular_file() && (entry.path().extension().string() == ".lua"))
			{
				LuaFile_T stLuaFile = {
					false,
					false,
					entry.path()
				};

				vecLuaFiles.push_back(stLuaFile);
			}

			continue;
		}

		return vecLuaFiles;
	}

	void AutoRun()
	{
		Utils::LogDebug("Attempting Lua Auto Run");

		std::optional<std::filesystem::path> pathAutoRun = Utils::GetLuaFilePath("autorun");
		if (!pathAutoRun)
			return;

		if (std::filesystem::exists(pathAutoRun.value())) 
			ExecuteScript(pathAutoRun.value());

		return;
	}

	void ExecuteScript(std::string sScript)
	{
		int iResult = luaL_dostring(luaState, sScript.c_str());
		if (iResult != LUA_OK)
			Utils::LogError(lua_tostring(luaState, -1));
	}

	void ExecuteScript(std::filesystem::path pathFile)
	{
		int iResult = luaL_dofile(luaState, pathFile.string().c_str());
		if (iResult != LUA_OK)
			Utils::LogError(lua_tostring(luaState, -1));
	}

	void ExecuteLoaded()
	{
		for (auto luaFile : vecLoadedFiles) {
			ExecuteScript(luaFile.file);
		}
	}

	void LoadFile(LuaFile_T stLuaFile)
	{
		vecLoadedFiles.push_back(stLuaFile);
	}

	void UnloadFile(LuaFile_T stLuaFile)
	{
		bool bFound = false;
		int index = 0;

		for (int i = 0; i < vecLoadedFiles.size(); i++)
		{
			if (vecLoadedFiles[i].file == stLuaFile.file) {
				bFound = true;
				index = i;
				break;
			}
		}

		vecLoadedFiles.erase(vecLoadedFiles.begin() + index);
	}
};