#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include <shlobj.h>
#include <nlohmann/json.hpp>

class ElementBase;

class Config
{
public:
	static constexpr const char* kDefaultConfigName = "Default";

private:
	std::filesystem::path ConfigDir;
	std::string CurrentConfigName;

	std::filesystem::path GetManifestPath() const;
	std::filesystem::path GetConfigPath(const std::string& sName) const;

public:
	explicit Config(ElementBase& root);

	static std::string SanitizeName(const std::string& sName);

	bool SaveConfig(ElementBase& root);
	bool LoadConfig(ElementBase& root);

	bool SaveConfigAs(ElementBase& root, const std::string& sName);

	bool WriteConfigJson(const nlohmann::json& jsonConfig, const std::string& sName);

	bool LoadConfigNamed(ElementBase& root, const std::string& sName);

	bool RenameConfig(const std::string& sOldName, const std::string& sNewName);

	bool DeleteConfig(const std::string& sName);

	std::vector<std::string> ListConfigs() const;

	void EnsureDefaultValid();

	std::string GetDefaultConfigName() const;
	void SetDefaultConfig(const std::string& sName);

	void SetCurrentConfigName(const std::string& sName) { CurrentConfigName = SanitizeName(sName); };

	const std::string& GetCurrentConfigName() const { return CurrentConfigName; };
	const std::filesystem::path& GetConfigDir() const { return ConfigDir; };
};
