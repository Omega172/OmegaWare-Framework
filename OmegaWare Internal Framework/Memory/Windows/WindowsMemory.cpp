#include "pch.h"
#include "WindowsMemory.h"

static std::unordered_map<size_t, MODULEINFO> aCachedModules{};
LPMODULEINFO Memory::GetModuleInfo(std::string_view sModuleName)
{
	const size_t iModuleHash = std::hash<std::string_view>{}(sModuleName);
	if (auto itr = aCachedModules.find(iModuleHash); itr != aCachedModules.end())
		return &itr->second;

	HANDLE hHandle = GetPrivilegedHandleToProcess();
	if (!hHandle)
		return nullptr;

	HMODULE hModule = GetModuleHandle(sModuleName.data());
	if (!hModule)
		return nullptr;

	MODULEINFO mi;
	if (!GetModuleInformation(hHandle, hModule, &mi, sizeof(mi)))
		return nullptr;

	aCachedModules.emplace(iModuleHash, mi);
	// Too lazy to get the ptr to the info we just stored, so we just call the function again
	return GetModuleInfo(sModuleName);
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
		dwProcessId = Framework::wndproc->dwProcessId;

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
			size_t i = sModuleName.find_last_of("/\\");
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