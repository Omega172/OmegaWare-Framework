#include "pch.h"
#include <nlohmann/json.hpp>
#include <fstream>

Config::Config()
{
	std::stringstream SS;
	SS << Utils::GetDocumentsFolder() << "\\" << Framework::Framework << "\\";
	if (!std::filesystem::exists(SS.str()))
		std::filesystem::create_directory(SS.str());

	SS << FRAMEWORK_TARGET_GAME << ".cfg";
	ConfigPath = SS.str();

	LogDebugHere("Config Path: " + ConfigPath.string());

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
		LogErrorHere("Failed to open config file for writing");
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
		LogErrorHere("Failed to open config file for reading");
		return false;
	}

	std::stringstream ssContainer;
	ssContainer << fileConfig.rdbuf();
	nlohmann::json jsonConfig(nlohmann::json::parse(ssContainer.str()));
	fileConfig.close();

	Framework::menu->ConfigLoad(jsonConfig);

	return true;
}