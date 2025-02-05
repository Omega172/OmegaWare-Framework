#include "pch.h"
#include "Includes.h"

static std::optional<std::filesystem::path> GetFrameworkFolder()
{
	CHAR szFolder[MAX_PATH];
	if (!SUCCEEDED(SHGetFolderPathA(0, CSIDL_MYDOCUMENTS, 0, 0, szFolder))) {
		return {};
	}

	std::filesystem::path pathFramework{ szFolder };
	pathFramework = pathFramework / Framework::Framework / "";

	// Create the directory for this framework if it doesn't already exist, exit upon failure.
	if (!std::filesystem::exists(pathFramework)) {
		if (!std::filesystem::create_directory(pathFramework)) {
			return {};
		}
	}

	return pathFramework;
}


std::optional<std::filesystem::path> Utils::GetLogFilePath(const std::string& sFile, const std::string& sExtension)
{
	auto optPath = GetFrameworkFolder();
	if (!optPath) {
		return {};
	}
	
	std::filesystem::path pathLog = optPath.value();
	pathLog /= "Logs";
	pathLog /= "";

	// Create the directory for this log if it doesn't already exist, exit upon failure.
	if (!std::filesystem::exists(pathLog)) {
		if (!std::filesystem::create_directory(pathLog)) {
			return {};
		}
	}

	// They just want the folder path.
	if (sFile.size() == 0) {
		return pathLog;
	}

	pathLog += TARGET_GAME_NAME; // Prepend the framework target game name so it titles the log files correctly

	pathLog += sFile;
	if (sExtension.size() == 0) {
		return pathLog;
	}

	pathLog.replace_extension(sExtension);
	return pathLog;
}

std::optional<std::filesystem::path> Utils::GetConfigFilePath(const std::string& sFile, const std::string& sExtension)
{
	auto optPath = GetFrameworkFolder();
	if (!optPath) {
		return {};
	}

	std::filesystem::path pathConfig = optPath.value();
	pathConfig /= "Configs";
	pathConfig /= "";

	// Create the directory for this config if it doesn't already exist, exit upon failure.
	if (!std::filesystem::exists(pathConfig)) {
		if (!std::filesystem::create_directory(pathConfig)) {
			return {};
		}
	}

	// They just want the folder path.
	if (sFile.size() == 0) {
		return pathConfig;
	}

	pathConfig += sFile;
	if (sExtension.size() == 0) {
		return pathConfig;
	}

	pathConfig.replace_extension(sExtension);
	return pathConfig;
}

std::optional<std::filesystem::path> Utils::GetLuaFilePath(const std::string& sFile, const std::string& sExtension)
{
	auto optPath = GetFrameworkFolder();
	if (!optPath) {
		return {};
	}

	std::filesystem::path pathLua = optPath.value();
	pathLua /= "Lua";
	pathLua /= "";

	// Create the directory for this config if it doesn't already exist, exit upon failure.
	if (!std::filesystem::exists(pathLua)) {
		if (!std::filesystem::create_directory(pathLua)) {
			return {};
		}
	}

	// They just want the folder path.
	if (sFile.size() == 0) {
		return pathLua;
	}

	pathLua += sFile;
	if (sExtension.size() == 0) {
		return pathLua;
	}

	pathLua.replace_extension(sExtension);
	return pathLua;
}