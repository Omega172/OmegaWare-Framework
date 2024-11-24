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

void Memory::EnumerateInterfaces(std::string_view sModuleName, EnumerateInterfacesFunc fn)
{
	LPMODULEINFO pModuleInfo = GetModuleInfo(sModuleName);
	if (!pModuleInfo || !pModuleInfo->lpBaseOfDll || !pModuleInfo->SizeOfImage) {
		LogErrorStreamHere('[' << sModuleName << "] Unable to get Module Info!");
		return;
	}

	const uintptr_t CreateInterfaceSymbol = reinterpret_cast<uintptr_t>(GetProcAddress(static_cast<HMODULE>(pModuleInfo->lpBaseOfDll), "CreateInterface"));
	if (!CreateInterfaceSymbol) {
		LogErrorStreamHere('[' << sModuleName << "] Unable to get CreateInterface address!");
		return;
	}

	int32_t* pStartOffset = reinterpret_cast<int32_t*>(CreateInterfaceSymbol + 3);
	if (!IsValidObjectPtr(pStartOffset)) {
		LogErrorStreamHere('[' << sModuleName << "] Unable to get StartOffset!");
		return;
	}

	InterfaceRegistry_t** pInterfaceRegistryStart = reinterpret_cast<InterfaceRegistry_t**>(CreateInterfaceSymbol + *pStartOffset + 7);
	if (!IsValidPtr(pInterfaceRegistryStart)) {
		LogErrorStreamHere('[' << sModuleName << "] Unable to get InterfaceRegistryStart address!");
		return;
	}

	InterfaceRegistry_t* pInterfaceRegistry = *pInterfaceRegistryStart;
	for (; IsValidObjectPtr(pInterfaceRegistry); pInterfaceRegistry = pInterfaceRegistry->pNext) {
		if (!Strlen(pInterfaceRegistry->szName))
			continue;

		if (fn(pInterfaceRegistry))
			return;
	}
}

void* Memory::CreateInterface(std::string_view sModuleName, std::string_view sInterfaceName)
{
	void* rv = nullptr;

	EnumerateInterfaces(sModuleName, [&rv, sInterfaceName](InterfaceRegistry_t* pInterfaceRegistry) -> bool {
		if (std::string_view(pInterfaceRegistry->szName).find(sInterfaceName) == std::string_view::npos)
			return false;

		rv = pInterfaceRegistry->Create();
		return true;
	});

	if (!rv)
		LogErrorStreamHere('[' << sModuleName << "] Unable to get " << sInterfaceName);

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
	{
	}

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


void* Memory::SignatureScan(LPMODULEINFO pModuleInfo, SignatureSpan_t aSignature)
{
	if (!pModuleInfo || !pModuleInfo->lpBaseOfDll || !pModuleInfo->SizeOfImage)
		return nullptr;

	PUCHAR pMemory = reinterpret_cast<PUCHAR>(pModuleInfo->lpBaseOfDll);

	const SignatureElement_t* pSignature = aSignature.data();
	const size_t dwSignatureSize = aSignature.size();

	for (size_t i = 0; i < pModuleInfo->SizeOfImage - dwSignatureSize; i++) {
		bool bFound = true;
		for (size_t j = 0; j < dwSignatureSize; j++) {
			if (pMemory[i + j] != pSignature[j] && pSignature[j] != SignatureWildcardConvertedValue) {
				bFound = false;
				break;
			}
		}

		if (bFound)
			return reinterpret_cast<void*>(&pMemory[i]);
	}

	return nullptr;
}

void* Memory::MultiSignatureScan(std::string_view sModuleName, std::vector<SignatureData_t*> vecSignatures)
{
	LPMODULEINFO pModuleInfo = GetModuleInfo(sModuleName);
	if (!pModuleInfo)
		return nullptr;

	for (SignatureData_t* stSignatureData : vecSignatures)
	{
		void* rv = SignatureScan(pModuleInfo, stSignatureData->aSignature);
		if (!rv)
			continue;

		if (!stSignatureData->CorrectReturnAddressFunc)
			return rv;

		return stSignatureData->CorrectReturnAddressFunc(rv);
	}

	return nullptr;
}