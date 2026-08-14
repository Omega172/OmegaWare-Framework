#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include <shlobj.h>

class ElementBase;

// Takes the config root as a parameter rather than reaching for a global menu instance,
// so Shared has no knowledge of whichever Internal/Proxy target owns the actual menu tree.
class Config
{
private:
	std::filesystem::path ConfigPath;

public:
	explicit Config(ElementBase& root);

	bool SaveConfig(ElementBase& root);

	bool LoadConfig(ElementBase& root);
};