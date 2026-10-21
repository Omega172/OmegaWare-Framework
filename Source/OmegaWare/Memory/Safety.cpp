#include "Memory/Safety.hpp"

#include <Windows.h>

namespace OmegaWare::Memory
{
	namespace
	{
		constexpr DWORD kReadable = PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY;
		constexpr DWORD kWritable = PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY;
		constexpr DWORD kExecutable = PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY;

		bool HasAccess(const void* pAddress, std::size_t uLength, DWORD uAllowed)
		{
			if (!pAddress || !uLength)
				return false;
			auto uCursor = reinterpret_cast<std::uintptr_t>(pAddress);
			const std::uintptr_t uEnd = uCursor + uLength;
			if (uEnd < uCursor)
				return false;
			while (uCursor < uEnd)
			{
				MEMORY_BASIC_INFORMATION stInfo{};
				if (!VirtualQuery(reinterpret_cast<const void*>(uCursor), &stInfo, sizeof(stInfo)))
					return false;
				if (stInfo.State != MEM_COMMIT || (stInfo.Protect & (PAGE_GUARD | PAGE_NOACCESS)) || !(stInfo.Protect & uAllowed))
					return false;
				uCursor = reinterpret_cast<std::uintptr_t>(stInfo.BaseAddress) + stInfo.RegionSize;
			}
			return true;
		}

		template <typename T>
		std::size_t SafeLength(const T* szText, std::size_t uMaxLength)
		{
			if (!szText)
				return 0;
			for (std::size_t uLength = 0; uLength < uMaxLength; ++uLength)
			{
				const T* pCharacter = szText + uLength;
				if ((uLength == 0 || (reinterpret_cast<std::uintptr_t>(pCharacter) & 0xFFF) < sizeof(T)) && !IsReadable(pCharacter, sizeof(T)))
					return 0;
				if (*pCharacter == T{})
					return uLength;
			}
			return uMaxLength;
		}
	}

	bool IsReadable(const void* pAddress, std::size_t uLength)
	{
		return HasAccess(pAddress, uLength, kReadable);
	}

	bool IsWritable(const void* pAddress, std::size_t uLength)
	{
		return HasAccess(pAddress, uLength, kWritable);
	}

	bool IsExecutable(const void* pAddress)
	{
		return HasAccess(pAddress, 1, kExecutable);
	}

	std::size_t SafeStrlen(const char* szText, std::size_t uMaxLength)
	{
		return SafeLength(szText, uMaxLength);
	}

	std::size_t SafeWcslen(const wchar_t* szText, std::size_t uMaxLength)
	{
		return SafeLength(szText, uMaxLength);
	}

	std::size_t GetVirtualMethodCount(const void* pObject, std::size_t uMaxCount)
	{
		if (!IsValidPtr(static_cast<void* const*>(pObject)))
			return 0;
		void* const* pTable = *static_cast<void* const* const*>(pObject);
		std::size_t uCount = 0;
		while (uCount < uMaxCount && IsValidPtr(pTable + uCount) && IsExecutable(pTable[uCount]))
			++uCount;
		return uCount;
	}

	void* GetVirtualMethod(const void* pObject, std::size_t uIndex)
	{
		if (!IsValidPtr(static_cast<void* const*>(pObject)))
			return nullptr;
		void* const* pTable = *static_cast<void* const* const*>(pObject);
		if (!IsValidPtr(pTable + uIndex) || !IsExecutable(pTable[uIndex]))
			return nullptr;
		return pTable[uIndex];
	}
}
