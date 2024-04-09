#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include <shlobj.h>

class Config
{
private:
	std::filesystem::path ConfigPath;

public:
	Config();

	bool SaveConfig();

	bool LoadConfig();
};