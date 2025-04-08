#pragma once

#include "pch.h"
#include "WindowsTypes.hpp"

namespace Memory
{
	// Gets loaded module info
	LPMODULEINFO GetModuleInfo(std::string_view sModuleName);
	LPMODULEINFO GetModuleInfo(HMODULE hModule);

	// Type used for EnumerateHandles function
	using EnumerateHandlesFunc = std::function<bool(PSYSTEM_HANDLE_TABLE_ENTRY_INFO)>;
	// Enumerates open handles with fn that returns true if it wants to stop the enumeration
	void EnumerateHandles(EnumerateHandlesFunc fn);

	// Get a handle to current process with PROCESS_QUERY_INFORMATION and PROCESS_VM_READ
	HANDLE GetPrivilegedHandleToProcess(DWORD dwProcessId = 0);

	void ReleaseHandles();

	// Type used for EnumerateModules function
	using EnumerateModulesFunc = std::function<bool(std::string_view)>;
	// Enum used for EnumerateModules function, filters the strings provided to the callback fn
	namespace EnumerateModulesFlags {
		constexpr DWORD DiscardSystemModules = 1 << 0; // No Windows Modules
		constexpr DWORD ModuleNameOnly = 1 << 1; // Instead of full path
		constexpr DWORD LowercaseName = 1 << 2; // Passes a fully lowercase name
	}

	// Enumerates process moduleswith fn that returns true if it wants to stop the enumeration
	void EnumerateModules(EnumerateModulesFunc fn, DWORD dwProcessId = 0, DWORD flags = EnumerateModulesFlags::DiscardSystemModules | EnumerateModulesFlags::ModuleNameOnly);

	/**
	 * Generates a random writable address in a discrete fashion.
	 * 
	 * \returns A pointer to the allocated memory.
	 * 
	 * \param size The size of memory needed to be allocated.
	 */
	void* GenerateRandomWritableAddress(size_t size);

	/**
	 * Creates a hidden dummy thread that will call our target function.
	 * 
	 * \param pTargetAddress The target function to call in a separate thread.
	 * 
	 * \param hModule The module that this function belongs to.
	 */
	void SpoofThreadAddress(void* pTargetAddress, HMODULE& hModule);
}