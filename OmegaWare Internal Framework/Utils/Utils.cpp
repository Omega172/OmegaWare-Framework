#include "pch.h"

#include <stdexcept>

std::string Utils::GetDocumentsFolder()
{
	char Folder[MAX_PATH];
	HRESULT hr = SHGetFolderPathA(0, CSIDL_MYDOCUMENTS, 0, 0, Folder);
	if (SUCCEEDED(hr))
		return Folder;

	else return "";
}