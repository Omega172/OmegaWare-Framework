#include "pch.h"
#include <fstream>
#include <cctype>
#include "../Utils/Logging/Logging.hpp"
#include "../GUI/Widgets/Elements.hpp"

std::filesystem::path Config::GetManifestPath() const
{
	return ConfigDir / "DefaultConfig.txt";
}

std::filesystem::path Config::GetConfigPath(const std::string& sName) const
{
	std::filesystem::path pathConfig = ConfigDir / sName;
	pathConfig.replace_extension("cfg");
	return pathConfig;
}

std::string Config::SanitizeName(const std::string& sName)
{
	std::string sSanitized;
	sSanitized.reserve(sName.size());

	static const std::string sReservedChars = "<>:\"/\\|?*";
	for (const char c : sName)
	{
		if (static_cast<unsigned char>(c) < 32 || sReservedChars.find(c) != std::string::npos)
			sSanitized += '_';
		else
			sSanitized += c;
	}
	while (!sSanitized.empty() && (sSanitized.back() == '.' || sSanitized.back() == ' '))
		sSanitized.pop_back();

	constexpr size_t kMaxLength = 100;
	if (sSanitized.size() > kMaxLength)
		sSanitized.resize(kMaxLength);

	static const char* const szReservedNames[] = {
		"CON", "PRN", "AUX", "NUL",
		"COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7", "COM8", "COM9",
		"LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9",
	};

	std::string sUpper = sSanitized;
	std::transform(sUpper.begin(), sUpper.end(), sUpper.begin(), [](unsigned char c) { return static_cast<char>(std::toupper(c)); });

	for (const char* szReserved : szReservedNames)
	{
		if (sUpper != szReserved)
			continue;

		sSanitized += '_';
		break;
	}

	return sSanitized;
}

Config::Config(ElementBase& root)
{
	{
		auto optPath = Utils::GetConfigFilePath();
		if (!optPath) {
			Utils::LogError("Utils::GetConfigFilePath failure! (Unable to initialize config system)");
			return;
		}

		ConfigDir = optPath.value();
	}

	CurrentConfigName = GetDefaultConfigName();

	Utils::LogDebug(std::format("Config Dir: {} | Default Config: {}", ConfigDir.string(), CurrentConfigName));

	if (!std::filesystem::exists(GetConfigPath(CurrentConfigName)))
	{
		SaveConfig(root);
		SetDefaultConfig(CurrentConfigName);
		return;
	}

	LoadConfig(root);
}

bool Config::SaveConfig(ElementBase& root)
{
	return SaveConfigAs(root, CurrentConfigName);
}

bool Config::SaveConfigAs(ElementBase& root, const std::string& sName)
{
	nlohmann::json jsonConfig;
	root.ConfigSave(jsonConfig);

	if (!WriteConfigJson(jsonConfig, sName))
		return false;

	CurrentConfigName = SanitizeName(sName);
	return true;
}

bool Config::WriteConfigJson(const nlohmann::json& jsonConfig, const std::string& sName)
{
	const std::string sSafeName = SanitizeName(sName);
	if (sSafeName.empty())
		return false;

	std::ofstream fileConfig(GetConfigPath(sSafeName));
	if (fileConfig.fail())
	{
		Utils::LogError("Failed to open config file for writing!");
		return false;
	}

	fileConfig << jsonConfig.dump(4);
	fileConfig.close();

	return true;
}

bool Config::LoadConfig(ElementBase& root)
{
	return LoadConfigNamed(root, CurrentConfigName);
}

bool Config::LoadConfigNamed(ElementBase& root, const std::string& sName)
{
	const std::string sSafeName = SanitizeName(sName);

	std::ifstream fileConfig(GetConfigPath(sSafeName));
	if (fileConfig.fail())
	{
		Utils::LogError("Failed to open config file for reading!");
		return false;
	}

	std::stringstream ssContainer;
	ssContainer << fileConfig.rdbuf();
	nlohmann::json jsonConfig(nlohmann::json::parse(ssContainer.str()));
	fileConfig.close();

	root.ConfigLoad(jsonConfig);

	CurrentConfigName = sSafeName;
	return true;
}

bool Config::RenameConfig(const std::string& sOldName, const std::string& sNewName)
{
	const std::string sSafeOld = SanitizeName(sOldName);
	const std::string sSafeNew = SanitizeName(sNewName);

	if (sSafeOld.empty() || sSafeNew.empty() || sSafeOld == sSafeNew)
		return false;

	std::filesystem::path pathNew = GetConfigPath(sSafeNew);
	if (std::filesystem::exists(pathNew))
		return false;

	std::error_code ec;
	std::filesystem::rename(GetConfigPath(sSafeOld), pathNew, ec);
	if (ec)
		return false;

	if (GetDefaultConfigName() == sSafeOld)
		SetDefaultConfig(sSafeNew);

	if (CurrentConfigName == sSafeOld)
		CurrentConfigName = sSafeNew;

	return true;
}

bool Config::DeleteConfig(const std::string& sName)
{
	std::error_code ec;
	return std::filesystem::remove(GetConfigPath(SanitizeName(sName)), ec);
}

std::vector<std::string> Config::ListConfigs() const
{
	std::vector<std::string> vecNames;

	if (!std::filesystem::exists(ConfigDir))
		return vecNames;

	for (const auto& entry : std::filesystem::directory_iterator(ConfigDir))
	{
		if (!entry.is_regular_file() || entry.path().extension() != ".cfg")
			continue;

		vecNames.push_back(entry.path().stem().string());
	}

	std::sort(vecNames.begin(), vecNames.end());
	return vecNames;
}

void Config::EnsureDefaultValid()
{
	std::vector<std::string> vecConfigs = ListConfigs();
	if (vecConfigs.size() == 1 && GetDefaultConfigName() != vecConfigs.front())
		SetDefaultConfig(vecConfigs.front());
}

std::string Config::GetDefaultConfigName() const
{
	std::filesystem::path pathManifest = GetManifestPath();
	if (std::filesystem::exists(pathManifest))
	{
		std::ifstream fileManifest(pathManifest);
		std::string sName;
		std::getline(fileManifest, sName);

		if (!sName.empty())
			return sName;
	}

	return kDefaultConfigName;
}

void Config::SetDefaultConfig(const std::string& sName)
{
	std::ofstream fileManifest(GetManifestPath(), std::ios::trunc);
	fileManifest << SanitizeName(sName);
}
