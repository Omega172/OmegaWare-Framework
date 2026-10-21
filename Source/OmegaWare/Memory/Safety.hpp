#pragma once

#include "OmegaRenderHook/Core/Guarded.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>
#include <type_traits>

namespace OmegaWare::Memory
{
	[[nodiscard]] bool IsReadable(const void* pAddress, std::size_t uLength);
	[[nodiscard]] bool IsWritable(const void* pAddress, std::size_t uLength);
	[[nodiscard]] bool IsExecutable(const void* pAddress);

	template <typename T>
	[[nodiscard]] bool IsValidPtr(const T* pAddress)
	{
		return IsReadable(pAddress, sizeof(void*));
	}

	template <typename T>
	[[nodiscard]] bool IsValidObjectPtr(const T* pAddress)
	{
		return IsReadable(pAddress, sizeof(T));
	}

	template <typename T>
		requires std::is_trivially_copyable_v<T>
	[[nodiscard]] std::optional<T> SafeRead(std::uintptr_t uAddress)
	{
		if (!uAddress)
			return std::nullopt;
		T tValue{};
		OmegaRenderHook::stFault stFault;
		auto fnRead = [&] { std::memcpy(&tValue, reinterpret_cast<const void*>(uAddress), sizeof(T)); };
		if (!OmegaRenderHook::InvokeGuarded(fnRead, stFault))
			return std::nullopt;
		return tValue;
	}

	[[nodiscard]] std::size_t SafeStrlen(const char* szText, std::size_t uMaxLength = 1024);
	[[nodiscard]] std::size_t SafeWcslen(const wchar_t* szText, std::size_t uMaxLength = 1024);

	[[nodiscard]] std::size_t GetVirtualMethodCount(const void* pObject, std::size_t uMaxCount = 1024);
	[[nodiscard]] void* GetVirtualMethod(const void* pObject, std::size_t uIndex);
}
