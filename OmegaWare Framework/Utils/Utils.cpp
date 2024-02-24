#include "pch.h"

#include <stdexcept>

// https://stackoverflow.com/questions/48708440/check-if-i-can-write-to-memory
// https://stackoverflow.com/questions/18394647/can-i-check-if-memory-block-is-readable-without-raising-exception-with-c
bool Utils::IsReadableMemory(void* lpAddress, size_t dwLength)
{
	MEMORY_BASIC_INFORMATION MemInfo;
	if (VirtualQuery(lpAddress, &MemInfo, sizeof(MEMORY_BASIC_INFORMATION)) == NULL)
	{
#ifdef EXCEPT_ON_VQUERY_ERR
		throw std::runtime_error(std::system_category().message(GetLastError()));
#endif
		return false;
	}

	if (MemInfo.State != MEM_COMMIT)
	{
#ifdef EXCEPT_ON_MEM_ERR
		throw std::runtime_error("State != MEM_COMMIT");
#endif
		return false;
	}

	if (MemInfo.Protect == PAGE_NOACCESS || MemInfo.Protect == PAGE_EXECUTE)
	{
#ifdef EXCEPT_ON_MEM_ERR
		throw std::runtime_error("Protect is not Readable");
#endif
		return false;
	}

	SIZE_T dwRemainingRegionSize = MemInfo.RegionSize + ((char*)lpAddress - (char*)MemInfo.AllocationBase);
	if (dwRemainingRegionSize < dwLength)
		return IsReadableMemory((char*)lpAddress + dwRemainingRegionSize, dwLength - dwRemainingRegionSize);

	return true;
}

bool Utils::IsReadableMemory(const void* lpAddress, size_t dwLength)
{
	MEMORY_BASIC_INFORMATION MemInfo;
	if (VirtualQuery(lpAddress, &MemInfo, sizeof(MEMORY_BASIC_INFORMATION)) == NULL)
	{
#ifdef EXCEPT_ON_VQUERY_ERR
		throw std::runtime_error(std::system_category().message(GetLastError()));
#endif
		return false;
	}

	if (MemInfo.State != MEM_COMMIT)
	{
#ifdef EXCEPT_ON_MEM_ERR
		throw std::runtime_error("State != MEM_COMMIT");
#endif
		return false;
	}

	if (MemInfo.Protect == PAGE_NOACCESS || MemInfo.Protect == PAGE_EXECUTE)
	{
#ifdef EXCEPT_ON_MEM_ERR
		throw std::runtime_error("Protect is not Readable");
#endif
		return false;
	}

	SIZE_T dwRemainingRegionSize = MemInfo.RegionSize + ((char*)lpAddress - (char*)MemInfo.AllocationBase);
	if (dwRemainingRegionSize < dwLength)
		return IsReadableMemory((char*)lpAddress + dwRemainingRegionSize, dwLength - dwRemainingRegionSize);

	return true;
}

size_t Utils::Strlen(const char* lpAddress, size_t dwMaxSize) {
	static constexpr size_t dwTypeSize = sizeof(char);

	MEMORY_BASIC_INFORMATION MemInfo;
	if (VirtualQuery(lpAddress, &MemInfo, sizeof(MEMORY_BASIC_INFORMATION)) == NULL)
	{
#ifdef EXCEPT_ON_VQUERY_ERR
		throw std::runtime_error(std::system_category().message(GetLastError()));
#endif
		return 0;
	}

	if (MemInfo.State != MEM_COMMIT)
	{
#ifdef EXCEPT_ON_MEM_ERR
		throw std::runtime_error("State != MEM_COMMIT");
#endif
		return 0;
	}

	if (MemInfo.Protect == PAGE_NOACCESS || MemInfo.Protect == PAGE_EXECUTE)
	{
#ifdef EXCEPT_ON_MEM_ERR
		throw std::runtime_error("Protect is not Readable");
#endif
		return 0;
	}

	size_t dwRemainingRegionSize = MemInfo.RegionSize + lpAddress - static_cast<char*>(MemInfo.AllocationBase);
	size_t dwSize = 0;
	while (1) {
		if (dwRemainingRegionSize + dwSize * dwTypeSize < dwTypeSize)
			break;

		if (!lpAddress[dwSize])
			return dwSize;

		++dwSize;

#ifdef FAIL_ON_MAX_STRLEN
		if (dwSize > dwMaxSize)
			return 0;
	}

	if (dwMaxSize - dwSize <= 1)
		return 0;

	size_t dwSizeNext = Strlen(lpAddress + dwSize * dwTypeSize, dwMaxSize - dwSize);
	if (!dwSizeNext)
		return 0;

	return dwSize + dwSizeNext;
#else
		if (dwSize > dwMaxSize)
			return dwMaxSize;
	}

	return dwSize + strlen(lpAddress + dwSize * dwTypeSize, dwMaxSize - dwSize);
#endif
}

size_t Utils::Wcslen(const wchar_t* lpAddress, size_t dwMaxSize) {
	static constexpr size_t dwTypeSize = sizeof(wchar_t);

	MEMORY_BASIC_INFORMATION MemInfo;
	if (VirtualQuery(lpAddress, &MemInfo, sizeof(MEMORY_BASIC_INFORMATION)) == NULL)
	{
#ifdef EXCEPT_ON_VQUERY_ERR
		Utils::LogError(Utils::GetLocation(CurrentLoc), GetLastError());
		throw std::runtime_error(std::system_category().message(GetLastError()));
#endif
		return 0;
	}

	if (MemInfo.State != MEM_COMMIT)
	{
#ifdef EXCEPT_ON_MEM_ERR
		Utils::LogError(Utils::GetLocation(CurrentLoc), GetLastError());
		throw std::runtime_error("State != MEM_COMMIT");
#endif
		return 0;
	}

	if (MemInfo.Protect == PAGE_NOACCESS || MemInfo.Protect == PAGE_EXECUTE)
	{
#ifdef EXCEPT_ON_MEM_ERR
		Utils::LogError(Utils::GetLocation(CurrentLoc), GetLastError());
		throw std::runtime_error("Protect is not Readable");
#endif
		return 0;
	}

	size_t dwRemainingRegionSize = MemInfo.RegionSize + lpAddress - static_cast<wchar_t*>(MemInfo.AllocationBase);
	size_t dwSize = 0;
	while (1) {
		if (dwRemainingRegionSize + dwSize * dwTypeSize < dwTypeSize)
			break;

		if (!lpAddress[dwSize])
			return dwSize;

		++dwSize;

#ifdef FAIL_ON_MAX_STRLEN
		if (dwSize > dwMaxSize)
			return 0;
	}

	if (dwMaxSize - dwSize <= 1)
		return 0;

	size_t dwSizeNext = Wcslen(lpAddress + dwSize * dwTypeSize, dwMaxSize - dwSize);
	if (!dwSizeNext)
		return 0;

	return dwSize + dwSizeNext;
#else
		if (dwSize > dwMaxSize)
			return dwMaxSize;
	}

	return dwSize + wcslen(lpAddress + dwSize * dwTypeSize, dwMaxSize - dwSize);
#endif
}

void* Utils::GetVirtualMethod(void* ptr, size_t index)
{
	if (!IsValidPtr(ptr))
		return nullptr;

	void** pMethodTable = *static_cast<void***>(ptr);
	if (!IsValidPtr(pMethodTable))
		return nullptr;

	void* pMethod = pMethodTable[index];
	return (IsValidPtr(pMethod)) ? pMethod : nullptr;
}

std::string Utils::GetDocumentsFolder()
{
	char Folder[MAX_PATH];
	HRESULT hr = SHGetFolderPathA(0, CSIDL_MYDOCUMENTS, 0, 0, Folder);
	if (SUCCEEDED(hr))
		return Folder;

	else return "";
}