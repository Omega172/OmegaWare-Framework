#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include <shlobj.h>

struct ConfigEntry
{
	std::string Name;
	std::string Type;
	std::string Value;
};

class Config
{
private:
	std::filesystem::path ConfigPath;

	std::string GetDocumentsFolder()
	{
		char Folder[MAX_PATH];
		HRESULT hr = SHGetFolderPathA(0, CSIDL_MYDOCUMENTS, 0, 0, Folder);
		if (SUCCEEDED(hr))
			return Folder;

		else return "";
	}

public:
	Config();

	bool SaveConfig();

	bool LoadConfig();

	void PushEntry(std::string Name, std::string Type, std::string Value);
	ConfigEntry GetEntryByName(std::string Name);
};