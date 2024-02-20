#pragma once
#include "Logging/Logging.h" // Include the logging header file
#include "Console/Console.h" // Include the console header file which contains the console class used to create a console window

#define IsValidObjectPtr(lpAddress) (lpAddress && Utils::IsReadableMemory(lpAddress, sizeof(*lpAddress)))
#define MemoryIsReadable(lpAddress) Utils::IsReadableMemory(lpAddress, sizeof(*lpAddress))

namespace Utils
{
	// Functions to check if memory is readable
	bool IsReadableMemory(void* ptr, size_t byteCount);
	bool IsReadableMemory(const void* ptr, size_t byteCount); 

	size_t Strlen(char*    lpAddress, size_t dwMaxSize = 1024);
	size_t Wcslen(wchar_t* lpAddress, size_t dwMaxSize = 1024);

	std::string GetDocumentsFolder();
}