#include "pch.h"
#include "WindowsMemory.hpp"

#include <random>

HMODULE Memory::GetModule(std::string_view sModuleName)
{
	return GetModuleHandleA(sModuleName.data());
}

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

LPMODULEINFO Memory::GetModuleInfo(HMODULE hModule)
{
	char szFilePath[1024]{};
	if (GetModuleFileNameA(hModule, szFilePath, sizeof(szFilePath)) == 0)
		return nullptr;

	std::filesystem::path pathModule{ szFilePath };
	return GetModuleInfo(pathModule.filename().string());
}

void Memory::EnumerateHandles(EnumerateHandlesFunc fn)
{
	using NtQuerySystemInformation_t = NTSTATUS(NTAPI*)(
		_In_ SYSTEM_INFORMATION_CLASS SystemInformationClass,
		_Out_writes_bytes_opt_(SystemInformationLength) PVOID SystemInformation,
		_In_ ULONG SystemInformationLength,
		_Out_opt_ PULONG ReturnLength
	);
	
	auto NtQuerySystemInformation = reinterpret_cast<NtQuerySystemInformation_t>(GetProcAddress(GetModuleHandle("ntdll.dll"), "NtQuerySystemInformation"));
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

static std::map<DWORD, HANDLE> mapHandles{};
HANDLE Memory::GetPrivilegedHandleToProcess(DWORD dwProcessId)
{
	if (!dwProcessId)
		dwProcessId = Framework::wndproc->dwProcessId;

	if (mapHandles.contains(dwProcessId))
		return mapHandles.find(dwProcessId)->second;

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

	if (rv)
		return rv;
	
	rv = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, false, dwProcessId);
	if (rv)
		mapHandles.emplace(dwProcessId, rv);

	return rv;
}

void Memory::ReleaseHandles() {
	for (auto& pairEntry : mapHandles) {
		CloseHandle(pairEntry.second);
	}
}

void Memory::EnumerateModules(EnumerateModulesFunc fn, DWORD dwProcessId, DWORD flags)
{
	HANDLE hProcess = GetPrivilegedHandleToProcess(dwProcessId);
	if (!hProcess)
		return;

	HMODULE hModules[1024];
	DWORD dwNeeded{};
	if (!EnumProcessModules(hProcess, hModules, sizeof(hModules), &dwNeeded))
		return;

	TCHAR szModuleName[1024];
	for (UINT i = 0; i < (dwNeeded / sizeof(HMODULE)); i++)
	{
		if (!GetModuleFileNameEx(hProcess, hModules[i], szModuleName, sizeof(szModuleName)))
			continue;

		std::string sModuleName(szModuleName);

		// We dont want to enumerate our own cheat dummy!
		if (CRC64::hash(sModuleName) == Framework::iModuleNameHash)
			continue;

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

void* Memory::GenerateRandomWritableAddress(size_t size) {
	// Get the base address and size of the current module
	HMODULE hModule = GetModuleHandle(NULL);
	if (!hModule) {
		std::cerr << "Failed to get module handle." << std::endl;
		return {};
	}

	MODULEINFO moduleInfo;
	if (!GetModuleInformation(GetCurrentProcess(), hModule, &moduleInfo, sizeof(moduleInfo))) {
		std::cerr << "Failed to get module information." << std::endl;
		return {};
	}

	uintptr_t iBaseAddress = reinterpret_cast<uintptr_t>(moduleInfo.lpBaseOfDll);

	// Ensure the random address is near the end of the memory page
	uintptr_t iWritableEnd = iBaseAddress + moduleInfo.SizeOfImage;
	uintptr_t iWritableStart = iWritableEnd - 4096; // Standard page size (4 KB)
	size_t sizeWritable = iWritableEnd - iWritableStart;

	if (sizeWritable < size) {
		Utils::LogError(std::format("Unable to find writable address of size {}", size));
		return {};
	}
	
	// Generate the random address making sure to leave room for the desired size.
	uintptr_t iRandomAddress = iWritableStart + Utils::Random<size_t>(0, sizeWritable - size);

	// Check if the address is valid and within the bounds of the module
	if (iRandomAddress < iWritableStart || iRandomAddress + size >= iWritableEnd) {
		Utils::LogError(std::format("Address {:#010x} of size {} is outside of bounds [{:#010x}, {:#010x}]", iRandomAddress, size, iWritableStart, iWritableEnd));
		return {};
	}

	// Check if we can change the memory protection
	MEMORY_BASIC_INFORMATION mbi;
	if (!VirtualQuery(reinterpret_cast<void*>(iRandomAddress), &mbi, sizeof(mbi))) {
		Utils::LogError(GetLastError());
		return {};
	}

	if (mbi.Protect & (PAGE_GUARD | PAGE_NOACCESS)) {
		Utils::LogError("Generated address is protected!");
		return {};
	}
	
	if (!VirtualProtect(reinterpret_cast<void*>(iRandomAddress), size, PAGE_EXECUTE_READ, {})) {
		Utils::LogError(GetLastError());
		return {};
	}

	return reinterpret_cast<void*>(iRandomAddress);
}

void Memory::SpoofThreadAddress(void* pTargetAddress, HMODULE& hModule)
{
	HMODULE hNTDll = GetModuleHandleA("ntdll");
	HANDLE hCurrentProcess = GetCurrentProcess();
	if (!hNTDll || !hCurrentProcess) {
		// There should probably be some error logic here.
		return;
	}

	// Get the RtlCreateUserThread function, it is not directly avaliable via the winapi headers.
	using RtlCreateUserThread_t = NTSTATUS(WINAPI*)(HANDLE, SECURITY_DESCRIPTOR*, BOOLEAN, SIZE_T, SIZE_T*, SIZE_T*, void*, void*, HANDLE*, void*);
	auto fnRtlCreateUserThread = reinterpret_cast<RtlCreateUserThread_t>(GetProcAddress(hNTDll, "RtlCreateUserThread"));
	if (!fnRtlCreateUserThread) {
		// There should probably be some error logic here.
		return;
	}

	// Log process memory info for testing purposes.
	PROCESS_MEMORY_COUNTERS_EX pmc{};
	if (K32GetProcessMemoryInfo(hCurrentProcess, (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
		std::cout << "Private Mem: 0x" << std::hex << pmc.PrivateUsage << std::endl;
	}

	// Generate a random address.
	auto pNewAddress = reinterpret_cast<PTHREAD_START_ROUTINE>(GenerateRandomWritableAddress(0x3e8));
	if (!pNewAddress) {
		// There should probably be some error logic here.
		return;
	}

	// Create a thread using our cool random memory address.
	HANDLE hThread{};
	if (FAILED(fnRtlCreateUserThread(hCurrentProcess, NULL, TRUE, NULL, NULL, NULL, pNewAddress, hModule, &hThread, NULL)) || !hThread) {
		// There should probably be some error logic here.
		return;
	}

	// Get the thread's context so we can modify the start address.
	CONTEXT ctxThread{ .ContextFlags = CONTEXT_INTEGER | CONTEXT_CONTROL };
	if (!GetThreadContext(hThread, &ctxThread)) {
		// There should probably be some error logic here.
		return;
	}

	// Modify the start address (Eip on x86, Rip on x64)
	ctxThread.Rip = reinterpret_cast<ULONG64>(pTargetAddress);
	ctxThread.ContextFlags = CONTEXT_INTEGER | CONTEXT_CONTROL;
	Utils::LogDebug(std::format("ctxThread.Rip: {:#010x}", ctxThread.Rip));

	// Set the thread context and resume the thread.
	if (!SetThreadContext(hThread, &ctxThread) || ResumeThread(hThread) == -1) {
		// There should probably be some error logic here.
		return;
	}
}