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

	return dwSize + strnlen_s(lpAddress + dwSize * dwTypeSize, dwMaxSize - dwSize);
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

	return dwSize + wcsnlen_s(lpAddress + dwSize * dwTypeSize, dwMaxSize - dwSize);
#endif
}

void Memory::EnumerateInterfaces(std::string_view svModuleName, std::function<bool(InterfaceRegistry_t*)> fn)
{
	LPMODULEINFO pModuleInfo = GetModuleInfo(svModuleName);
	if (!pModuleInfo || !pModuleInfo->lpBaseOfDll || !pModuleInfo->SizeOfImage) {
		Utils::LogError(std::format("[{}] Memory::GetModuleInfo failure!", svModuleName));
		return;
	}

	const uintptr_t CreateInterfaceSymbol = reinterpret_cast<uintptr_t>(GetProcAddress(static_cast<HMODULE>(pModuleInfo->lpBaseOfDll), "CreateInterface"));
	if (!CreateInterfaceSymbol) {
		Utils::LogError(std::format("[{}] Unable to find CreateInterface symbol!", svModuleName));
		return;
	}

	int32_t* pStartOffset = reinterpret_cast<int32_t*>(CreateInterfaceSymbol + 3);
	if (!IsValidObjectPtr(pStartOffset)) {
		Utils::LogError(std::format("[{}] pStartOffset is invalid!", svModuleName));
		return;
	}

	InterfaceRegistry_t** pInterfaceRegistryStart = reinterpret_cast<InterfaceRegistry_t**>(CreateInterfaceSymbol + *pStartOffset + 7);
	if (!IsValidPtr(pInterfaceRegistryStart)) {
		Utils::LogError(std::format("[{}] pInterfaceRegistryStart is invalid!", svModuleName));
		return;
	}

	InterfaceRegistry_t* pInterfaceRegistry = *pInterfaceRegistryStart;
	for (; IsValidObjectPtr(pInterfaceRegistry); pInterfaceRegistry = pInterfaceRegistry->m_pNext) {
		if (!Strlen(pInterfaceRegistry->m_szName))
			continue;

		if (fn(pInterfaceRegistry))
			return;
	}
}

void* Memory::CreateInterface(std::string_view svModuleName, std::string_view svInterfaceName)
{
	void* rv = nullptr;

	EnumerateInterfaces(svModuleName, [&rv, svInterfaceName](InterfaceRegistry_t* pInterfaceRegistry) -> bool {
		if (std::string_view(pInterfaceRegistry->m_szName).find(svInterfaceName) == std::string_view::npos)
			return false;

		rv = pInterfaceRegistry->m_fnCreate();
		return true;
	});

	if (!rv)
		Utils::LogError(std::format("[{}] Unable to find interface \"{}\"", svModuleName, svInterfaceName));

	return rv;
}

size_t Memory::GetVirtualMethodTableSize(void* lpAddress)
{
	if (!IsValidPtr(lpAddress))
		return 0;

	void** pMethodTable = *static_cast<void***>(lpAddress);
	if (!IsValidPtr(pMethodTable))
		return 0;

	size_t rv = 0;
	while (IsValidPtr(pMethodTable[rv++]))
	{}

	return rv;
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

static void* ScanModuleForSignature(const LPMODULEINFO pModuleInfo, const Memory::SignatureData_t::Span_t aSignature) 
{
	const auto pMemory = reinterpret_cast<PUCHAR>(pModuleInfo->lpBaseOfDll);
	const auto pSignature = aSignature.data();
	const auto dwSignatureSize = aSignature.size();

	for (size_t i = 0; i < pModuleInfo->SizeOfImage - dwSignatureSize; ++i) {
		bool bFound = true;
		for (size_t j = 0; j < dwSignatureSize; ++j) {
			if (pMemory[i + j] != pSignature[j] && pSignature[j] != -1) {
				bFound = false;
				break;
			}
		}

		if (bFound)
			return reinterpret_cast<void*>(&pMemory[i]);
	}

	return nullptr;
}

void* Memory::SignatureScan(const std::string_view svModuleName, const SignatureData_t::Span_t aSignature) 
{
	LPMODULEINFO pModuleInfo = GetModuleInfo(svModuleName);
	if (!pModuleInfo || !pModuleInfo->lpBaseOfDll || !pModuleInfo->SizeOfImage)
		return nullptr;

	return ScanModuleForSignature(pModuleInfo, aSignature);
}

void* Memory::SignatureScan(const HMODULE hModule, const SignatureData_t::Span_t aSignature)
{
	LPMODULEINFO pModuleInfo = GetModuleInfo(hModule);
	if (!pModuleInfo || !pModuleInfo->lpBaseOfDll || !pModuleInfo->SizeOfImage)
		return nullptr;

	return ScanModuleForSignature(pModuleInfo, aSignature);
}

void* Memory::SignatureScan(const std::string_view svModuleName, const std::vector<SignatureData_t> vecSignatures)
{
	LPMODULEINFO pModuleInfo = GetModuleInfo(svModuleName);
	if (!pModuleInfo || !pModuleInfo->lpBaseOfDll || !pModuleInfo->SizeOfImage)
		return nullptr;

	for (auto& stSignatureData : vecSignatures)
	{
		void* rv = ScanModuleForSignature(pModuleInfo, stSignatureData.aSignature);
		if (!rv)
			continue;

		if (!stSignatureData.CorrectReturnAddressFunc)
			return rv;

		return reinterpret_cast<void*>(stSignatureData.CorrectReturnAddressFunc(reinterpret_cast<uintptr_t>(rv)));
	}

	return nullptr;
}

void* Memory::SignatureScan(const HMODULE hModule, const std::vector<SignatureData_t> vecSignatures)
{
	LPMODULEINFO pModuleInfo = GetModuleInfo(hModule);
	if (!pModuleInfo || !pModuleInfo->lpBaseOfDll || !pModuleInfo->SizeOfImage)
		return nullptr;

	for (auto& stSignatureData : vecSignatures)
	{
		void* rv = ScanModuleForSignature(pModuleInfo, stSignatureData.aSignature);
		if (!rv)
			continue;

		if (!stSignatureData.CorrectReturnAddressFunc)
			return rv;

		return reinterpret_cast<void*>(stSignatureData.CorrectReturnAddressFunc(reinterpret_cast<uintptr_t>(rv)));
	}

	return nullptr;
}

static std::vector<void*> GetAllInstancesOfSignature(PUCHAR pMemory, size_t size, Memory::SignatureData_t::Span_t aSignature, size_t sizeLimit)
{
	std::vector<void*> vecReturned{};

	const auto pSignature = aSignature.data();
	const auto dwSignatureSize = aSignature.size();

	for (size_t i = 0; i < size - dwSignatureSize; ++i) {
		bool bFound = true;
		for (size_t j = 0; j < dwSignatureSize; ++j) {
			if (pMemory[i + j] != pSignature[j] && pSignature[j] != -1) {
				bFound = false;
				break;
			}
		}

		if (bFound) {
			vecReturned.emplace_back(reinterpret_cast<void*>(&pMemory[i]));
			if (vecReturned.size() >= sizeLimit)
				break;
		}
	}

	return vecReturned;
}

std::vector<Memory::ModuleScanResult_t> Memory::SignatureScan(std::vector<std::string_view> vecModules, bool bIncludeProcess,
	const std::vector<SignatureData_t> vecSignatures, size_t sizePerModuleLimit)
{
	std::vector<Memory::ModuleScanResult_t> vecReturned{};

	// Start by going through each module first.
	for (auto& svModuleName : vecModules) {
		LPMODULEINFO pModuleInfo = GetModuleInfo(svModuleName);
		if (!pModuleInfo || !pModuleInfo->lpBaseOfDll || !pModuleInfo->SizeOfImage)
			continue;

		auto& stScanResult = vecReturned.emplace_back();
		stScanResult.m_sModuleName = svModuleName;

		// Look for the patterns!
		for (auto& stSignatureData : vecSignatures) {
			auto vecPointers = GetAllInstancesOfSignature(reinterpret_cast<PUCHAR>(pModuleInfo->lpBaseOfDll), pModuleInfo->SizeOfImage,
				stSignatureData.aSignature, sizePerModuleLimit - stScanResult.m_vecPointers.size());

			// Apply the correction function where applicable
			if (stSignatureData.CorrectReturnAddressFunc)
				for (auto& ptr : vecPointers)
					ptr = reinterpret_cast<void*>(stSignatureData.CorrectReturnAddressFunc(reinterpret_cast<uintptr_t>(ptr)));

			// Add our new collection of pointers and stop scanning this module if we have enough.
			stScanResult.m_vecPointers.insert(stScanResult.m_vecPointers.end(), vecPointers.begin(), vecPointers.end());
			if (stScanResult.m_vecPointers.size() >= sizePerModuleLimit)
				break;
		}

		// This entry is empty, remove it.
		if (stScanResult.m_vecPointers.size() == 0)
			vecReturned.erase(--vecReturned.end());
	}

	// Now scan the process
	if (!bIncludeProcess)
		return vecReturned;

	HANDLE hProcess = GetPrivilegedHandleToProcess();
	
	// Get the process name.
	char szProcessName[1024]{};
	if (FAILED(GetProcessImageFileName(hProcess, szProcessName, sizeof(szProcessName))))
		return vecReturned;

	// Get the main module of the process.
	LPMODULEINFO pModuleInfo = GetModuleInfo(std::filesystem::path(szProcessName).filename().string());
	if (!pModuleInfo || !pModuleInfo->lpBaseOfDll || !pModuleInfo->SizeOfImage)
		return vecReturned;

	auto& stScanResult = vecReturned.emplace_back();
	stScanResult.m_sModuleName = std::filesystem::path(szProcessName).filename().string();

	// Look for the patterns!
	for (auto& stSignatureData : vecSignatures) {
		auto vecPointers = GetAllInstancesOfSignature(reinterpret_cast<PUCHAR>(pModuleInfo->lpBaseOfDll), pModuleInfo->SizeOfImage,
			stSignatureData.aSignature, sizePerModuleLimit - stScanResult.m_vecPointers.size());

		// Apply the correction function where applicable
		if (stSignatureData.CorrectReturnAddressFunc)
			for (auto& ptr : vecPointers)
				ptr = reinterpret_cast<void*>(stSignatureData.CorrectReturnAddressFunc(reinterpret_cast<uintptr_t>(ptr)));

		// Add our new collection of pointers and stop scanning this module if we have enough.
		stScanResult.m_vecPointers.insert(stScanResult.m_vecPointers.end(), vecPointers.begin(), vecPointers.end());
		if (stScanResult.m_vecPointers.size() >= sizePerModuleLimit)
			break;
	}

	// This entry is empty, remove it.
	if (stScanResult.m_vecPointers.size() == 0)
		vecReturned.erase(--vecReturned.end());

	return vecReturned;
}

std::vector<Memory::ModuleScanResult_t> Memory::SignatureScan(const std::vector<SignatureData_t> vecSignatures, size_t sizePerModuleLimit)
{
	std::vector<Memory::ModuleScanResult_t> vecReturned{};

	// Start by going through each module first.
	EnumerateModules([&vecReturned, vecSignatures, sizePerModuleLimit](auto svModuleName) -> bool {
		LPMODULEINFO pModuleInfo = GetModuleInfo(svModuleName);
		if (!pModuleInfo || !pModuleInfo->lpBaseOfDll || !pModuleInfo->SizeOfImage)
			return false;

		auto& stScanResult = vecReturned.emplace_back();
		stScanResult.m_sModuleName = svModuleName;

		// Look for the patterns!
		for (auto& stSignatureData : vecSignatures) {
			auto vecPointers = GetAllInstancesOfSignature(reinterpret_cast<PUCHAR>(pModuleInfo->lpBaseOfDll), pModuleInfo->SizeOfImage,
				stSignatureData.aSignature, sizePerModuleLimit - stScanResult.m_vecPointers.size());

			// Apply the correction function where applicable
			if (stSignatureData.CorrectReturnAddressFunc)
				for (auto& ptr : vecPointers)
					ptr = reinterpret_cast<void*>(stSignatureData.CorrectReturnAddressFunc(reinterpret_cast<uintptr_t>(ptr)));

			// Add our new collection of pointers and stop scanning this module if we have enough.
			stScanResult.m_vecPointers.insert(stScanResult.m_vecPointers.end(), vecPointers.begin(), vecPointers.end());
			if (stScanResult.m_vecPointers.size() >= sizePerModuleLimit)
				break;
		}

		// This entry is empty, remove it.
		if (stScanResult.m_vecPointers.size() == 0)
			vecReturned.erase(--vecReturned.end());

		return false;
	});

	return vecReturned;
}

static std::vector<void*> g_vecTrampolineCollection{};
bool Memory::ResetTrampolineCollection()
{
	g_vecTrampolineCollection.clear();
	for (auto& stScanResult : SignatureScan({ { Signature("FF 23"), {} } }))
	{
		MEMORY_BASIC_INFORMATION mbi{};
		for (auto& ptr : stScanResult.m_vecPointers)
		{
			if (VirtualQuery(ptr, &mbi, sizeof(mbi)) == 0)
				continue;

			if(mbi.Protect == PAGE_EXECUTE_READ)
				g_vecTrampolineCollection.emplace_back(ptr);
		}
	}

	Utils::LogDebug(std::format("Trampolines collected: {}", g_vecTrampolineCollection.size()));
	return g_vecTrampolineCollection.size() > 0;
}

void* Memory::GetRandomTrampoline()
{
	assert(g_vecTrampolineCollection.size() != 0);
	return g_vecTrampolineCollection.at(Utils::Random<size_t>(0, g_vecTrampolineCollection.size() - 1));
}