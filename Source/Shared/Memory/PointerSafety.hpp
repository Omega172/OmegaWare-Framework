#pragma once

#include <cstddef>
#include <cstdint>

namespace Memory
{
	/**
	 * Determine if the memory being pointed to is readable to prevent access violations.
	 *
	 * \warning Can be expensive!
	 */
	bool IsReadable(const void* lpAddress, size_t dwLength);

	/**
	 * Checks that a pointer-sized read at lpAddress is safe (e.g. before dereferencing a slot
	 * that is itself expected to hold a pointer, such as a vtable entry).
	 */
	template<typename T>
	inline bool IsValidPtr(const T* lpAddress)
	{
		return IsReadable(lpAddress, sizeof(void*));
	}

	/**
	 * Checks that a read of the full pointed-to object at lpAddress is safe.
	 */
	template<typename T>
	inline bool IsValidObjectPtr(const T* lpAddress)
	{
		return IsReadable(lpAddress, sizeof(*lpAddress));
	}

	/**
	 * \returns The number of methods in the virtual method table.
	 */
	size_t GetVirtualMethodTableSize(void* lpAddress);

	/**
	 * \returns A pointer to the method at the given index in the virtual method table, or nullptr if invalid.
	 */
	void* GetVirtualMethod(void* lpAddress, size_t index);
}
