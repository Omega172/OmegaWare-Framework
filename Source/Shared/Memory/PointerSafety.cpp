#include "pch.h"
#include "PointerSafety.hpp"

// https://stackoverflow.com/questions/48708440/check-if-i-can-write-to-memory
// https://stackoverflow.com/questions/18394647/can-i-check-if-memory-block-is-readable-without-raising-exception-with-c
bool Memory::IsReadable(const void* lpAddress, size_t dwLength)
{
	if (!lpAddress)
		return false;

	MEMORY_BASIC_INFORMATION MemInfo;
	if (VirtualQuery(lpAddress, &MemInfo, sizeof(MEMORY_BASIC_INFORMATION)) == NULL)
		return false;

	if (MemInfo.State != MEM_COMMIT)
		return false;

	if (MemInfo.Protect == PAGE_NOACCESS || MemInfo.Protect == PAGE_EXECUTE)
		return false;

	SIZE_T dwRemainingRegionSize = MemInfo.RegionSize + ((char*)lpAddress - (char*)MemInfo.AllocationBase);
	if (dwRemainingRegionSize < dwLength)
		return IsReadable((char*)lpAddress + dwRemainingRegionSize, dwLength - dwRemainingRegionSize);

	return true;
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
