#include "pch.h"
#include <fstream>
#include "../Utils/Logging/Logging.hpp"
#include "../GUI/Widgets/Elements.hpp"

Config::Config(ElementBase& root)
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
		SaveConfig(root);
		return;
	}

	LoadConfig(root);
	return;
}

bool Config::SaveConfig(ElementBase& root)
{
	std::ofstream fileConfig(ConfigPath);
	if (fileConfig.fail())
	{
		Utils::LogError("Failed to open config file for writing!");
		return false;
	}

	nlohmann::json jsonConfig;
	root.ConfigSave(jsonConfig);

	fileConfig << jsonConfig.dump(4);
	fileConfig.close();
	return true;
}

bool Config::LoadConfig(ElementBase& root)
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

	root.ConfigLoad(jsonConfig);

	return true;
}