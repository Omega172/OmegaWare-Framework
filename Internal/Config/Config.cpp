#include "pch.h"
#include "../Libs/Nlohmann.json/json.hpp"
#include <fstream>
#include "../Utils/Logging/Logging.h"

Config::Config()
{
	std::filesystem::path pathConfig{};
	{
		auto optPath = Utils::GetConfigFilePath(TARGET_GAME_NAME, "cfg");
		if (!optPath) {
			Utils::LogError("Utils::GetConfigFilePath failure! (Unable to initialize config system)");
			return;
		}

		pathConfig = optPath.value();
	}

	ConfigPath = pathConfig;

	Utils::LogDebug(std::format("Config Path: {}", ConfigPath.string()));

	if (!std::filesystem::exists(ConfigPath))
	{
		SaveConfig();
		return;
	}

	LoadConfig();
	return;
}

bool Config::SaveConfig()
{
	std::ofstream fileConfig(ConfigPath);
	if (fileConfig.fail())
	{
		Utils::LogError("Failed to open config file for writing!");
		return false;
	}

	nlohmann::json jsonConfig;
	Framework::menu->ConfigSave(jsonConfig);

	fileConfig << jsonConfig.dump(4);
	fileConfig.close();
	return true;
}

bool Config::LoadConfig()
{
	std::ifstream fileConfig(ConfigPath);
	if (fileConfig.fail())
	{
		Utils::LogError("Failed to open config filed for reading!");
		return false;
	}

	std::stringstream ssContainer;
	ssContainer << fileConfig.rdbuf();
	nlohmann::json jsonConfig(nlohmann::json::parse(ssContainer.str()));
	fileConfig.close();

	Framework::menu->ConfigLoad(jsonConfig);

	return true;
}