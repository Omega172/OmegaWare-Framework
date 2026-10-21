#pragma once

#include <filesystem>

namespace OmegaWare::CrashDump
{
	void Install(const std::filesystem::path& pathDirectory);
	void Uninstall();
}
