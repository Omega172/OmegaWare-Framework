#include "pch.h"
#include <shellapi.h>

static std::optional<std::string> GetExecutableName()
{
	CHAR szExePath[MAX_PATH];
	if (!GetModuleFileNameA(NULL, szExePath, MAX_PATH)) {
		return {};
	}

	return std::filesystem::path(szExePath).stem().string();
}

static std::optional<std::filesystem::path> GetFrameworkFolder()
{
	CHAR szFolder[MAX_PATH];
	if (!SUCCEEDED(SHGetFolderPathA(0, CSIDL_MYDOCUMENTS, 0, 0, szFolder))) {
		return {};
	}

	auto optExeName = GetExecutableName();
	if (!optExeName) {
		return {};
	}

	std::filesystem::path pathFramework{ szFolder };
	pathFramework = pathFramework / FRAMEWORK_CODENAME / optExeName.value() / "";

	if (!std::filesystem::exists(pathFramework)) {
		if (!std::filesystem::create_directories(pathFramework)) {
			return {};
		}
	}

	return pathFramework;
}

static const std::string& GetLaunchTimestamp()
{
	static const std::string sTimestamp = std::format("{:%Y-%m-%d_%H-%M-%S}", std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now()));
	return sTimestamp;
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

	if (!std::filesystem::exists(pathLog)) {
		if (!std::filesystem::create_directory(pathLog)) {
			return {};
		}
	}

	if (sFile.size() == 0) {
		return pathLog;
	}

	pathLog += GetLaunchTimestamp(); // One log file per launch instead of one shared file appended to forever.

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

	if (!std::filesystem::exists(pathConfig)) {
		if (!std::filesystem::create_directory(pathConfig)) {
			return {};
		}
	}

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

	if (!std::filesystem::exists(pathLua)) {
		if (!std::filesystem::create_directory(pathLua)) {
			return {};
		}
	}

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

bool Utils::OpenFolder(const std::filesystem::path& pathFolder)
{
	if (!std::filesystem::exists(pathFolder)) {
		return false;
	}

	HINSTANCE hResult = ShellExecuteW(NULL, L"explore", pathFolder.c_str(), NULL, NULL, SW_SHOWNORMAL);
	return reinterpret_cast<INT_PTR>(hResult) > 32;
}