#pragma once

#include <filesystem>
#include <optional>

#include "Console/Console.h"
#include "Logging/Logging.h"

namespace Utils
{
	std::optional<std::filesystem::path> GetLogFilePath(const std::string& sFile = "", const std::string& sExtension = "");

	std::optional<std::filesystem::path> GetConfigFilePath(const std::string& sFile = "", const std::string& sExtension = "");
}