// SHGetFolderPathA
#include <shlobj_core.h>

#include "Utils.h"
#include "pch.h"

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

	// They just want the folder path.
	if (sFile.size() == 0) {
		return pathLog;
	}

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

	std::filesystem::path pathLog = optPath.value();
	pathLog /= "Configs";
	pathLog /= "";

	// They just want the folder path.
	if (sFile.size() == 0) {
		return pathLog;
	}

	pathLog += sFile;
	if (sExtension.size() == 0) {
		return pathLog;
	}

	pathLog.replace_extension(sExtension);
	return pathLog;
}