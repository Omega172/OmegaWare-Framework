#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include <shlobj.h>

struct ConfigEntry
{
	std::string Name;
	std::string Type;
	std::string Value;
};

class Config
{
private:
	std::filesystem::path ConfigPath;

public:
	Config();

	bool SaveConfig();

	bool LoadConfig();

	void PushEntry(std::string Name, std::string Type, std::string Value);
	ConfigEntry GetEntryByName(std::string Name);
};