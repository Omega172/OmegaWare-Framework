#include "Memory/Memory.hpp"
#include "Core/Log.hpp"

#include <TlHelp32.h>

#include <algorithm>
#include <array>
#include <cstring>

namespace OmegaWare::Memory
{
	namespace
	{
		constexpr DWORD kReadable = PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY;

		[[nodiscard]] std::wstring Widen(std::string_view strText)
		{
			if (strText.empty())
				return {};
			const int iLength = MultiByteToWideChar(CP_UTF8, 0, strText.data(), static_cast<int>(strText.size()), nullptr, 0);
			std::wstring strWide(static_cast<std::size_t>(iLength), L'\0');
			MultiByteToWideChar(CP_UTF8, 0, strText.data(), static_cast<int>(strText.size()), strWide.data(), iLength);
			return strWide;
		}

		[[nodiscard]] std::string Narrow(std::wstring_view strText)
		{
			if (strText.empty())
				return {};
			const int iLength = WideCharToMultiByte(CP_UTF8, 0, strText.data(), static_cast<int>(strText.size()), nullptr, 0, nullptr, nullptr);
			std::string strNarrow(static_cast<std::size_t>(iLength), '\0');
			WideCharToMultiByte(CP_UTF8, 0, strText.data(), static_cast<int>(strText.size()), strNarrow.data(), iLength, nullptr, nullptr);
			return strNarrow;
		}

		[[nodiscard]] std::optional<stModule> DescribeModule(HMODULE hModule)
		{
			if (!hModule)
				return std::nullopt;
			const auto uBase = reinterpret_cast<std::uintptr_t>(hModule);
			const auto pDos = reinterpret_cast<const IMAGE_DOS_HEADER*>(uBase);
			if (!IsValidObjectPtr(pDos) || pDos->e_magic != IMAGE_DOS_SIGNATURE)
				return std::nullopt;
			const auto pNt = reinterpret_cast<const IMAGE_NT_HEADERS*>(uBase + static_cast<std::uintptr_t>(pDos->e_lfanew));
			if (!IsValidObjectPtr(pNt) || pNt->Signature != IMAGE_NT_SIGNATURE)
				return std::nullopt;

			wchar_t szPath[MAX_PATH]{};
			GetModuleFileNameW(hModule, szPath, MAX_PATH);
			std::wstring_view strPath(szPath);
			const std::size_t uSlash = strPath.find_last_of(L"\\/");
			return stModule{ Narrow(uSlash == std::wstring_view::npos ? strPath : strPath.substr(uSlash + 1)), hModule, uBase, pNt->OptionalHeader.SizeOfImage };
		}

		[[nodiscard]] bool Matches(const std::uint8_t* pBytes, std::span<const PatternByte> aPattern)
		{
			for (std::size_t i = 0; i < aPattern.size(); ++i)
				if (aPattern[i] != kWildcard && pBytes[i] != static_cast<std::uint8_t>(aPattern[i]))
					return false;
			return true;
		}

		void ScanRange(std::uintptr_t uStart, std::uintptr_t uEnd, std::span<const PatternByte> aPattern, std::vector<std::uintptr_t>& rResults, std::size_t uLimit)
		{
			if (uEnd - uStart < aPattern.size())
				return;
			const auto itAnchor = std::ranges::find_if(aPattern, [](PatternByte iByte) { return iByte != kWildcard; });
			const std::size_t uAnchor = itAnchor == aPattern.end() ? 0 : static_cast<std::size_t>(itAnchor - aPattern.begin());
			const auto pBegin = reinterpret_cast<const std::uint8_t*>(uStart);
			const std::size_t uLast = uEnd - uStart - aPattern.size();

			std::size_t uOffset = 0;
			while (uOffset <= uLast && rResults.size() < uLimit)
			{
				if (itAnchor != aPattern.end())
				{
					const void* pFound = std::memchr(pBegin + uOffset + uAnchor, static_cast<std::uint8_t>(*itAnchor), uLast - uOffset + 1);
					if (!pFound)
						return;
					uOffset = static_cast<std::size_t>(static_cast<const std::uint8_t*>(pFound) - pBegin) - uAnchor;
				}
				if (Matches(pBegin + uOffset, aPattern))
					rResults.push_back(uStart + uOffset);
				++uOffset;
			}
		}

		void Scan(std::uintptr_t uStart, std::size_t uSize, std::span<const PatternByte> aPattern, std::vector<std::uintptr_t>& rResults, std::size_t uLimit)
		{
			if (aPattern.empty() || !uStart || !uSize)
				return;
			const std::uintptr_t uEnd = uStart + uSize;
			std::uintptr_t uRunStart = 0;
			std::uintptr_t uCursor = uStart;
			while (uCursor < uEnd && rResults.size() < uLimit)
			{
				MEMORY_BASIC_INFORMATION stInfo{};
				if (!VirtualQuery(reinterpret_cast<const void*>(uCursor), &stInfo, sizeof(stInfo)))
					break;
				const std::uintptr_t uRegionEnd = std::min<std::uintptr_t>(reinterpret_cast<std::uintptr_t>(stInfo.BaseAddress) + stInfo.RegionSize, uEnd);
				const bool bReadable = stInfo.State == MEM_COMMIT && !(stInfo.Protect & (PAGE_GUARD | PAGE_NOACCESS)) && (stInfo.Protect & kReadable);
				if (bReadable && !uRunStart)
					uRunStart = uCursor;
				if (!bReadable && uRunStart)
				{
					ScanRange(uRunStart, uCursor, aPattern, rResults, uLimit);
					uRunStart = 0;
				}
				uCursor = uRegionEnd;
			}
			if (uRunStart)
				ScanRange(uRunStart, std::min(uCursor, uEnd), aPattern, rResults, uLimit);
		}
	}

	std::optional<stModule> GetModule(std::string_view strName)
	{
		const HMODULE hModule = strName.empty() ? GetModuleHandleW(nullptr) : GetModuleHandleW(Widen(strName).c_str());
		return DescribeModule(hModule);
	}

	std::vector<stModule> GetModules()
	{
		std::vector<stModule> aModules;
		const HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, GetCurrentProcessId());
		if (hSnapshot == INVALID_HANDLE_VALUE)
			return aModules;
		MODULEENTRY32W stEntry{ .dwSize = sizeof(MODULEENTRY32W) };
		for (BOOL bMore = Module32FirstW(hSnapshot, &stEntry); bMore; bMore = Module32NextW(hSnapshot, &stEntry))
			aModules.push_back({ Narrow(stEntry.szModule), stEntry.hModule, reinterpret_cast<std::uintptr_t>(stEntry.modBaseAddr), stEntry.modBaseSize });
		CloseHandle(hSnapshot);
		return aModules;
	}

	std::uintptr_t FindPattern(std::uintptr_t uStart, std::size_t uSize, std::span<const PatternByte> aPattern)
	{
		std::vector<std::uintptr_t> aResults;
		Scan(uStart, uSize, aPattern, aResults, 1);
		return aResults.empty() ? 0 : aResults.front();
	}

	std::vector<std::uintptr_t> FindAllPatterns(std::uintptr_t uStart, std::size_t uSize, std::span<const PatternByte> aPattern, std::size_t uLimit)
	{
		std::vector<std::uintptr_t> aResults;
		Scan(uStart, uSize, aPattern, aResults, uLimit);
		return aResults;
	}

	std::uintptr_t FindPattern(const stModule& stTarget, std::span<const PatternByte> aPattern)
	{
		return FindPattern(stTarget.uBase, stTarget.uSize, aPattern);
	}

	std::uintptr_t FindPattern(std::string_view strModule, std::span<const PatternByte> aPattern)
	{
		const std::optional<stModule> optModule = GetModule(strModule);
		if (!optModule)
		{
			LogWarning("module \"{}\" is not loaded", strModule);
			return 0;
		}
		return FindPattern(*optModule, aPattern);
	}

	std::uintptr_t FindFirst(const stModule& stTarget, std::span<const stSignature> aSignatures)
	{
		for (const stSignature& rSignature : aSignatures)
		{
			const std::uintptr_t uAddress = FindPattern(stTarget, rSignature.aPattern);
			if (!uAddress)
				continue;
			return rSignature.fnResolve ? rSignature.fnResolve(uAddress) : uAddress;
		}
		return 0;
	}

	std::vector<stModuleMatches> FindInAllModules(std::span<const PatternByte> aPattern, std::size_t uLimitPerModule)
	{
		std::vector<stModuleMatches> aMatches;
		for (const stModule& rModule : GetModules())
		{
			std::vector<std::uintptr_t> aAddresses = FindAllPatterns(rModule.uBase, rModule.uSize, aPattern, uLimitPerModule);
			if (!aAddresses.empty())
				aMatches.push_back({ rModule.strName, std::move(aAddresses) });
		}
		return aMatches;
	}

	std::uintptr_t ResolveRelative(std::uintptr_t uInstruction, std::size_t uDisplacementOffset, std::size_t uInstructionLength)
	{
		const std::optional<std::int32_t> optDisplacement = SafeRead<std::int32_t>(uInstruction + uDisplacementOffset);
		if (!optDisplacement)
			return 0;
		return uInstruction + uInstructionLength + static_cast<std::intptr_t>(*optDisplacement);
	}

	namespace
	{
		[[nodiscard]] bool LooksLikeRegistry(std::uintptr_t uHead)
		{
			const std::optional<std::uintptr_t> optFirst = SafeRead<std::uintptr_t>(uHead);
			if (!optFirst || !*optFirst)
				return false;
			const auto* pFirst = reinterpret_cast<const stInterfaceRegistry*>(*optFirst);
			if (!IsReadable(pFirst, sizeof(stInterfaceRegistry)))
				return false;
			return IsExecutable(reinterpret_cast<const void*>(pFirst->pfnCreate)) && SafeStrlen(pFirst->szName) > 0;
		}

		[[nodiscard]] std::uintptr_t FollowJumps(std::uintptr_t uCode)
		{
			for (int iDepth = 0; iDepth < 4; ++iDepth)
			{
				const std::optional<std::array<std::uint8_t, 5>> optBytes = SafeRead<std::array<std::uint8_t, 5>>(uCode);
				if (!optBytes)
					return uCode;
				const auto& aBytes = *optBytes;
				if (aBytes[0] == 0xE9)
					uCode = ResolveRelative(uCode, 1, 5);
				else if (aBytes[0] == 0x55 && aBytes[1] == 0x8B && aBytes[2] == 0xEC && aBytes[3] == 0x5D && aBytes[4] == 0xE9)
					uCode = ResolveRelative(uCode + 4, 1, 5);
				else
					return uCode;
			}
			return uCode;
		}

		[[nodiscard]] std::uintptr_t FindRegistryHead(std::uintptr_t uCreateInterface)
		{
			constexpr std::size_t kScanBytes = 64;
			const std::uintptr_t uCode = FollowJumps(uCreateInterface);
			const std::optional<std::array<std::uint8_t, kScanBytes + 8>> optBytes = SafeRead<std::array<std::uint8_t, kScanBytes + 8>>(uCode);
			if (!optBytes)
				return 0;
			const auto& aBytes = *optBytes;
			for (std::size_t i = 0; i < kScanBytes; ++i)
			{
#ifdef _WIN64
				const bool bRex = (aBytes[i] & 0xF8) == 0x48;
				const bool bRipLoad = bRex && aBytes[i + 1] == 0x8B && (aBytes[i + 2] & 0xC7) == 0x05;
				if (!bRipLoad)
					continue;
				const std::uintptr_t uHead = ResolveRelative(uCode + i, 3, 7);
#else
				std::uintptr_t uHead = 0;
				if (aBytes[i] == 0x8B && (aBytes[i + 1] & 0xC7) == 0x05)
					uHead = *reinterpret_cast<const std::uint32_t*>(&aBytes[i + 2]);
				else if (aBytes[i] == 0xA1)
					uHead = *reinterpret_cast<const std::uint32_t*>(&aBytes[i + 1]);
				else
					continue;
#endif
				if (LooksLikeRegistry(uHead))
					return uHead;
			}
			return 0;
		}

		using FnCreateInterface = void* (__cdecl*)(const char*, int*);
	}

	void EnumerateInterfaces(std::uintptr_t uCreateInterface, const std::function<bool(stInterfaceRegistry&)>& fnVisit)
	{
		const std::uintptr_t uRegistryHead = FindRegistryHead(uCreateInterface);
		if (!uRegistryHead)
			return LogWarning("could not find the interface registry behind CreateInterface at {:#x}", uCreateInterface);

		auto* pRegistry = *reinterpret_cast<stInterfaceRegistry**>(uRegistryHead);
		for (std::size_t uGuard = 0; IsValidObjectPtr(pRegistry) && uGuard < 4096; pRegistry = pRegistry->pNext, ++uGuard)
		{
			if (!SafeStrlen(pRegistry->szName))
				continue;
			if (fnVisit(*pRegistry))
				return;
		}
	}

	void EnumerateInterfaces(std::string_view strModule, const std::function<bool(stInterfaceRegistry&)>& fnVisit)
	{
		const std::optional<stModule> optModule = GetModule(strModule);
		if (!optModule)
			return LogWarning("module \"{}\" is not loaded", strModule);
		const auto uCreateInterface = reinterpret_cast<std::uintptr_t>(GetProcAddress(optModule->hModule, "CreateInterface"));
		if (!uCreateInterface)
			return LogWarning("module \"{}\" has no CreateInterface export", strModule);
		EnumerateInterfaces(uCreateInterface, fnVisit);
	}

	void* CreateInterface(std::string_view strModule, std::string_view strInterface)
	{
		const std::optional<stModule> optModule = GetModule(strModule);
		if (!optModule)
		{
			LogWarning("module \"{}\" is not loaded", strModule);
			return nullptr;
		}
		const auto pfnCreateInterface = reinterpret_cast<FnCreateInterface>(GetProcAddress(optModule->hModule, "CreateInterface"));
		if (!pfnCreateInterface)
		{
			LogWarning("module \"{}\" has no CreateInterface export", strModule);
			return nullptr;
		}

		const std::string strName(strInterface);
		int iReturnCode = 1;
		if (void* pInterface = pfnCreateInterface(strName.c_str(), &iReturnCode); pInterface && iReturnCode == 0)
			return pInterface;

		void* pInterface = nullptr;
		EnumerateInterfaces(reinterpret_cast<std::uintptr_t>(pfnCreateInterface), [&](stInterfaceRegistry& rRegistry) {
			if (std::string_view(rRegistry.szName).find(strInterface) == std::string_view::npos)
				return false;
			pInterface = IsExecutable(reinterpret_cast<const void*>(rRegistry.pfnCreate)) ? rRegistry.pfnCreate() : nullptr;
			return true;
		});
		if (!pInterface)
			LogWarning("interface \"{}\" not found in \"{}\"", strInterface, strModule);
		return pInterface;
	}
}
