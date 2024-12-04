#pragma once

#include <filesystem>
#include <optional>

#include "Console/Console.h"
#include "Logging/Logging.h"

namespace Utils
{
	/**
	 * Returns the path to the log file as requested, if sFile is empty then the directory path will be returned.
	 */
	std::optional<std::filesystem::path> GetLogFilePath(const std::string& sFile = "", const std::string& sExtension = "");

	/**
	 * Returns the path to the config file as requested, if sFile is empty then the directory path will be returned.
	 */
	std::optional<std::filesystem::path> GetConfigFilePath(const std::string& sFile = "", const std::string& sExtension = "");

	/**
	 * Returns the path to the lua file as requested, if sFile is empty then the directory path will be returned.
	 */
	std::optional<std::filesystem::path> GetLuaFilePath(const std::string& sFile = "", const std::string& sExtension = "lua");
}