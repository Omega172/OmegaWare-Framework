#pragma once

#include "pch.h"
#include "WindowsTypes.hpp"

namespace Memory
{
	HMODULE GetModule(std::string_view sModuleName);

	LPMODULEINFO GetModuleInfo(std::string_view sModuleName);
	LPMODULEINFO GetModuleInfo(HMODULE hModule);

	using EnumerateHandlesFunc = std::function<bool(PSYSTEM_HANDLE_TABLE_ENTRY_INFO)>;
	// Enumerates open handles with fn that returns true if it wants to stop the enumeration
	void EnumerateHandles(EnumerateHandlesFunc fn);

	// Get a handle to current process with PROCESS_QUERY_INFORMATION and PROCESS_VM_READ
	HANDLE GetPrivilegedHandleToProcess(DWORD dwProcessId = 0);

	void ReleaseHandles();

	using EnumerateModulesFunc = std::function<bool(std::string_view)>;
	namespace EnumerateModulesFlags {
		constexpr DWORD DiscardSystemModules = 1 << 0;
		constexpr DWORD ModuleNameOnly = 1 << 1;
		constexpr DWORD LowercaseName = 1 << 2;
	}

	// Enumerates process moduleswith fn that returns true if it wants to stop the enumeration
	void EnumerateModules(EnumerateModulesFunc fn, DWORD dwProcessId = 0, DWORD flags = EnumerateModulesFlags::DiscardSystemModules | EnumerateModulesFlags::ModuleNameOnly);
}
