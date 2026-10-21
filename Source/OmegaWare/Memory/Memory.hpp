#pragma once

#include "Memory/Pattern.hpp"
#include "Memory/Safety.hpp"

#include <Windows.h>

#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace OmegaWare::Memory
{
	struct stModule
	{
		std::string strName;
		HMODULE hModule = nullptr;
		std::uintptr_t uBase = 0;
		std::size_t uSize = 0;
	};

	struct stSignature
	{
		std::span<const PatternByte> aPattern;
		std::function<std::uintptr_t(std::uintptr_t)> fnResolve;
	};

	struct stModuleMatches
	{
		std::string strModule;
		std::vector<std::uintptr_t> aAddresses;
	};

	[[nodiscard]] std::optional<stModule> GetModule(std::string_view strName = {});
	[[nodiscard]] std::vector<stModule> GetModules();

	[[nodiscard]] std::uintptr_t FindPattern(std::uintptr_t uStart, std::size_t uSize, std::span<const PatternByte> aPattern);
	[[nodiscard]] std::vector<std::uintptr_t> FindAllPatterns(std::uintptr_t uStart, std::size_t uSize, std::span<const PatternByte> aPattern, std::size_t uLimit = SIZE_MAX);
	[[nodiscard]] std::uintptr_t FindPattern(const stModule& stTarget, std::span<const PatternByte> aPattern);
	[[nodiscard]] std::uintptr_t FindPattern(std::string_view strModule, std::span<const PatternByte> aPattern);
	[[nodiscard]] std::uintptr_t FindFirst(const stModule& stTarget, std::span<const stSignature> aSignatures);
	[[nodiscard]] std::vector<stModuleMatches> FindInAllModules(std::span<const PatternByte> aPattern, std::size_t uLimitPerModule = SIZE_MAX);

	[[nodiscard]] std::uintptr_t ResolveRelative(std::uintptr_t uInstruction, std::size_t uDisplacementOffset, std::size_t uInstructionLength);

	struct stInterfaceRegistry
	{
		void* (*pfnCreate)();
		const char* szName;
		stInterfaceRegistry* pNext;
	};

	void EnumerateInterfaces(std::string_view strModule, const std::function<bool(stInterfaceRegistry&)>& fnVisit);
	void EnumerateInterfaces(std::uintptr_t uCreateInterface, const std::function<bool(stInterfaceRegistry&)>& fnVisit);
	[[nodiscard]] void* CreateInterface(std::string_view strModule, std::string_view strInterface);
}
