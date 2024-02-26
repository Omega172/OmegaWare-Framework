#include "pch.h"

// https://stackoverflow.com/questions/48708440/check-if-i-can-write-to-memory
// https://stackoverflow.com/questions/18394647/can-i-check-if-memory-block-is-readable-without-raising-exception-with-c
bool Memory::IsReadable(const void* lpAddress, size_t dwLength)
{
	if (!lpAddress)
		return false;

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
		return IsReadable((char*)lpAddress + dwRemainingRegionSize, dwLength - dwRemainingRegionSize);

	return true;
}

size_t Memory::Strlen(const char* lpAddress, size_t dwMaxSize) {
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

size_t Memory::Wcslen(const wchar_t* lpAddress, size_t dwMaxSize) {
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

static std::unordered_map<int, HMODULE> aCachedModules{};
HMODULE Memory::GetModule(std::string sModuleName)
{
	int iModuleHash = std::hash<std::string>{}(sModuleName);
	if (auto itr = aCachedModules.find(iModuleHash); itr != aCachedModules.end())
		return itr->second;

	HMODULE module = GetModuleHandle(sModuleName.c_str());
	if (!module)
		return nullptr;

	aCachedModules.emplace(iModuleHash, module);
	return module;
}

void Memory::EnumerateHandles(EnumerateHandlesFunc fn)
{
	_NtQuerySystemInformation NtQuerySystemInformation = reinterpret_cast<_NtQuerySystemInformation>(GetProcAddress(GetModuleHandle("ntdll.dll"), "NtQuerySystemInformation"));
	if (!NtQuerySystemInformation)
		return;

	PSYSTEM_HANDLE_INFORMATION pSystemHandleInformation = nullptr;
	ULONG ulSize = 0;
	NTSTATUS lStatus = STATUS_UNSUCCESSFUL;

	// Wait until we successfully get the sysinfo
	while (1) {
		lStatus = NtQuerySystemInformation(SystemHandleInformation, pSystemHandleInformation, ulSize, &ulSize);
		if (NT_SUCCESS(lStatus) || lStatus != STATUS_INFO_LENGTH_MISMATCH)
			break;

		if (pSystemHandleInformation)
			VirtualFree(pSystemHandleInformation, 0, MEM_RELEASE);

		pSystemHandleInformation = reinterpret_cast<PSYSTEM_HANDLE_INFORMATION>(
			VirtualAlloc(NULL, ulSize, MEM_COMMIT, PAGE_READWRITE));
	}

	if (!pSystemHandleInformation)
		return;

	for (ULONG i = 0; i < pSystemHandleInformation->NumberOfHandles; i++)
		if (fn(&pSystemHandleInformation->Handles[i]))
			break;

	VirtualFree(pSystemHandleInformation, 0, MEM_RELEASE);
}

HANDLE Memory::GetPrivilegedHandleToProcess(DWORD dwProcessId)
{
	if (!dwProcessId)
		dwProcessId = Cheat::wndproc->dwProcessId;

	HANDLE rv = nullptr;

	EnumerateHandles([&rv, dwProcessId](PSYSTEM_HANDLE_TABLE_ENTRY_INFO pHandleEntryInfo) -> bool {
		if (dwProcessId != pHandleEntryInfo->ProcessId)
			return false;
		
		POBJECT_TYPE_INFORMATION pTypeInformation = nullptr;
		ULONG ulSize = 0x400;
		NTSTATUS lStatus = STATUS_INFO_LENGTH_MISMATCH;

		while (lStatus == STATUS_INFO_LENGTH_MISMATCH) {
			pTypeInformation = reinterpret_cast<POBJECT_TYPE_INFORMATION>(
				VirtualAlloc(NULL, ulSize, MEM_COMMIT, PAGE_READWRITE));
			lStatus = NtQueryObject(reinterpret_cast<HANDLE>(pHandleEntryInfo->Handle), ObjectTypeInformation, pTypeInformation, ulSize, &ulSize);

			if (NT_SUCCESS(lStatus))
				break;

			if (pTypeInformation)
				VirtualFree(pTypeInformation, 0, MEM_RELEASE);
		}


		if (!NT_SUCCESS(lStatus)) {
			if (pTypeInformation)
				VirtualFree(pTypeInformation, 0, MEM_RELEASE);

			return false;
		}

		int iComparison = std::wcscmp(pTypeInformation->TypeName.Buffer, L"Process");
		VirtualFree(pTypeInformation, 0, MEM_RELEASE);

		if (iComparison != 0)
			return false;

		HMODULE hModules[1024];
		DWORD dwNeeded;
		// This check will fail if the handle doesnt have access to PROCESS_QUERY_INFORMATION
		if (!EnumProcessModules(reinterpret_cast<HANDLE>(pHandleEntryInfo->Handle), hModules, sizeof(hModules), &dwNeeded))
			return false;

		// This check will fail if the handle doesnt have access to PROCESS_VM_READ
		TCHAR szProcessPath[MAX_PATH];
		if (!GetModuleFileNameEx(reinterpret_cast<HANDLE>(pHandleEntryInfo->Handle), NULL, szProcessPath, MAX_PATH))
			return false;

		rv = reinterpret_cast<HANDLE>(pHandleEntryInfo->Handle);
		return true;
	});

	return rv;
}

void Memory::EnumerateModules(EnumerateModulesFunc fn, DWORD dwProcessId, DWORD flags)
{
	HANDLE hProcess = GetPrivilegedHandleToProcess(dwProcessId);
	if (!hProcess)
		return;

	HMODULE hModules[1024];
	DWORD dwNeeded;
	if (!EnumProcessModules(hProcess, hModules, sizeof(hModules), &dwNeeded))
		return;

	TCHAR szModuleName[MAX_PATH];
	for (UINT i = 0; i < (dwNeeded / sizeof(HMODULE)); i++)
	{
		if (!GetModuleFileNameEx(hProcess, hModules[i], szModuleName, sizeof(szModuleName)))
			continue;

		std::string sModuleName(szModuleName);

		if (flags & EnumerateModulesFlags::DiscardSystemModules) {
			if (sModuleName.find(":\\WINDOWS\\") != std::string::npos || sModuleName.find(":\\Windows\\") != std::string::npos)
				continue;
		}

		if (flags & EnumerateModulesFlags::ModuleNameOnly) {
			int i = sModuleName.find_last_of("/\\");
			if (i != std::string::npos)
				sModuleName = sModuleName.substr(i + 1);
		}

		if (flags & EnumerateModulesFlags::LowercaseName) {
			std::transform(sModuleName.begin(), sModuleName.end(), sModuleName.begin(), [](unsigned char c) -> unsigned char {
				return std::tolower(c);
			});
		}

		if (fn(sModuleName))
			return;
	}
}

void* Memory::GetVirtualMethod(void* lpAddress, size_t index)
{
	if (!IsValidPtr(lpAddress))
		return nullptr;

	void** pMethodTable = *static_cast<void***>(lpAddress);
	if (!IsValidPtr(pMethodTable))
		return nullptr;

	void* pMethod = pMethodTable[index];
	return (IsValidPtr(pMethod)) ? pMethod : nullptr;
}
