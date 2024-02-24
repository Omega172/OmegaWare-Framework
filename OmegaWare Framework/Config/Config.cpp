#include "pch.h"
#include <nlohmann/json.hpp>
#include <fstream>

Config::Config()
{
	std::stringstream SS;
	SS << Utils::GetDocumentsFolder() << "\\" << Cheat::Framework << "\\";
	if (!std::filesystem::exists(SS.str()))
		std::filesystem::create_directory(SS.str());

	SS << FRAMEWORK_TARGET_GAME << ".cfg";
	ConfigPath = SS.str();

	Utils::LogDebug(Utils::GetLocation(CurrentLoc), "Config Path: " + ConfigPath.string());

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
	std::ofstream ConfigFile(ConfigPath);
	if (ConfigFile.fail())
	{
		Utils::LogError(Utils::GetLocation(CurrentLoc), "Failed to open config file for writing");
		return false;
	}

	nlohmann::json ConfigJson;
	ConfigJson["Config"] = nlohmann::json::array();

	Cheat::Entries.clear();

	PushEntry("Watermark", "bool", std::to_string(Cheat::bWatermark));
	PushEntry("WatermarkFPS", "bool", std::to_string(Cheat::bWatermarkFPS));

	for (size_t i = 0; i < Features.size(); i++)
	{
		Features[i]->SaveConfig();
	}

	for (ConfigEntry Entry : Cheat::Entries)
	{
		nlohmann::json EntryJson;
		EntryJson["Name"] = Entry.Name;
		EntryJson["Type"] = Entry.Type;
		EntryJson["Value"] = Entry.Value;
		ConfigJson["Config"].push_back(EntryJson);
	}

	ConfigFile << ConfigJson.dump(4);
	ConfigFile.close();
	return true;
}

bool Config::LoadConfig()
{
	Cheat::Entries.clear();

	std::ifstream ConfigFile(ConfigPath);
	if (ConfigFile.fail())
	{
		Utils::LogError(Utils::GetLocation(CurrentLoc), "Failed to open config file for reading");
		return false;
	}

	std::stringstream Container;
	Container << ConfigFile.rdbuf();
	ConfigFile.close();

	nlohmann::json ConfigJson(nlohmann::json::parse(Container.str()));

	for (nlohmann::json Entry : ConfigJson["Config"])
	{
		std::string Name = Entry["Name"];
		std::string Type = Entry["Type"];
		std::string Value = Entry["Value"];

		PushEntry(Name, Type, Value);
	}

	ConfigEntry WatermarkEntry = Cheat::config->GetEntryByName("Watermark");
	if (WatermarkEntry.Name == "Watermark")
		Cheat::bWatermark = std::stoi(WatermarkEntry.Value);

	ConfigEntry WatermarkFPSEntry = Cheat::config->GetEntryByName("WatermarkFPS");
	if (WatermarkFPSEntry.Name == "WatermarkFPS")
		Cheat::bWatermarkFPS = std::stoi(WatermarkFPSEntry.Value);

	for (size_t i = 0; i < Features.size(); i++)
	{
		Features[i]->LoadConfig();
	}

	return true;
}

void Config::PushEntry(std::string Name, std::string Type, std::string Value)
{
	ConfigEntry Entry;
	Entry.Name = Name;
	Entry.Type = Type;
	Entry.Value = Value;
	Cheat::Entries.push_back(Entry);
}

ConfigEntry Config::GetEntryByName(std::string Name)
{
	for (ConfigEntry Entry : Cheat::Entries)
	{
		if (Entry.Name == Name)
			return Entry;
	}

	ConfigEntry Entry;
	Entry.Name = "NULL";
	Entry.Type = "NULL";
	Entry.Value = "NULL";
	return Entry;
}