#include "Engines/Unreal/SdkAudit.hpp"

#if OMEGAWARE_ENGINE_UNREAL

#include "Core/Framework.hpp"
#include "Core/Log.hpp"
#include "Engines/Unreal/Resolver.hpp"
#include "Engines/Unreal/Unreal.hpp"

#include "OmegaRenderHook/Core/Guarded.hpp"

#include <Windows.h>

#include <cctype>
#include <charconv>
#include <chrono>
#include <format>
#include <fstream>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace OmegaWare::Unreal::SdkAudit
{
	namespace
	{
		std::mutex g_mtxAudit;

		[[nodiscard]] std::string_view EmbeddedTable()
		{
			HMODULE hModule = nullptr;
			if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCWSTR>(&EmbeddedTable), &hModule))
				return {};
			const HRSRC hResource = FindResourceW(hModule, L"OMEGAWARE_SDK_AUDIT", MAKEINTRESOURCEW(10));
			const HGLOBAL hData = hResource ? LoadResource(hModule, hResource) : nullptr;
			const void* pData = hData ? LockResource(hData) : nullptr;
			return pData ? std::string_view(static_cast<const char*>(pData), SizeofResource(hModule, hResource)) : std::string_view{};
		}

		[[nodiscard]] std::string PathName(const SDK::UObject* pObject)
		{
			std::vector<std::string> aParts;
			for (const SDK::UObject* pCursor = pObject; pCursor && aParts.size() < 16; pCursor = pCursor->Outer)
				aParts.push_back(pCursor->Name.ToString());
			std::string strPath;
			for (auto itPart = aParts.rbegin(); itPart != aParts.rend(); ++itPart)
			{
				if (!strPath.empty())
					strPath += '.';
				strPath += *itPart;
			}
			return strPath;
		}

		[[nodiscard]] std::unordered_map<std::string, const SDK::UStruct*> LoadedStructs()
		{
			std::unordered_map<std::string, const SDK::UStruct*> mapStructs;
			const std::int32_t iCount = SDK::UObject::GObjects->Num();
			for (std::int32_t i = 0; i < iCount; ++i)
			{
				const SDK::UObject* pObject = SDK::UObject::GObjects->GetByIndex(i);
				if (!pObject || !pObject->Class || !(static_cast<std::uint64_t>(pObject->Class->CastFlags) & static_cast<std::uint64_t>(SDK::EClassCastFlags::Struct)))
					continue;
				mapStructs.emplace(PathName(pObject), static_cast<const SDK::UStruct*>(pObject));
			}
			return mapStructs;
		}

		[[nodiscard]] std::string SdkIdentifier(std::string strName)
		{
			for (char& rCharacter : strName)
				if (!(std::isalnum(static_cast<unsigned char>(rCharacter)) || rCharacter == '_'))
					rCharacter = '_';
			return strName;
		}

		[[nodiscard]] std::string LooseName(std::string_view strName)
		{
			std::string strLoose;
			for (const char chValue : strName)
				if (std::isalnum(static_cast<unsigned char>(chValue)))
					strLoose += static_cast<char>(std::tolower(static_cast<unsigned char>(chValue)));
			return strLoose;
		}

		struct stReflected
		{
			std::unordered_map<std::string, std::size_t> mapExact;
			std::unordered_multimap<std::string, std::size_t> mapLoose;
		};

		[[nodiscard]] stReflected ReflectedOffsets(const SDK::UStruct* pStruct)
		{
			stReflected stOffsets;
			for (const SDK::FField* pField = pStruct->ChildProperties; pField; pField = pField->Next)
			{
				const std::size_t uOffset = static_cast<std::size_t>(static_cast<const SDK::FProperty*>(pField)->Offset);
				std::string strName = pField->Name.ToString();
				stOffsets.mapLoose.emplace(LooseName(strName), uOffset);
				std::string strIdentifier = SdkIdentifier(strName);
				stOffsets.mapExact.emplace(std::move(strName), uOffset);
				stOffsets.mapExact.emplace(std::move(strIdentifier), uOffset);
			}
			return stOffsets;
		}

		[[nodiscard]] bool MatchesRenamed(const stReflected& rOffsets, std::string_view strMember, std::size_t uSdkOffset)
		{
			if (strMember.starts_with("Param_") && MatchesRenamed(rOffsets, strMember.substr(6), uSdkOffset))
				return true;
			while (!strMember.empty())
			{
				const auto [itBegin, itEnd] = rOffsets.mapLoose.equal_range(LooseName(strMember));
				for (auto itOffset = itBegin; itOffset != itEnd; ++itOffset)
					if (itOffset->second == uSdkOffset)
						return true;
				const std::size_t uCut = strMember.find_last_of('_');
				if (uCut == std::string_view::npos)
					break;
				strMember = strMember.substr(0, uCut);
			}
			return false;
		}

		struct stIssue
		{
			std::string_view strOwner;
			std::string_view strMember;
			std::size_t uSdkOffset = 0;
			std::size_t uGameOffset = 0;
		};

		void Audit(std::string_view strTable, stSummary& rSummary, std::vector<stIssue>& rMoved, std::vector<stIssue>& rNotReflected, std::vector<std::string_view>& rMissing)
		{
			const auto mapStructs = LoadedStructs();
			std::string_view strOwner;
			const SDK::UStruct* pOwner = nullptr;
			stReflected stOffsets;
			std::size_t uCursor = 0;
			while (uCursor < strTable.size())
			{
				std::size_t uEnd = strTable.find('\n', uCursor);
				if (uEnd == std::string_view::npos)
					uEnd = strTable.size();
				std::string_view strLine = strTable.substr(uCursor, uEnd - uCursor);
				uCursor = uEnd + 1;
				if (strLine.ends_with('\r'))
					strLine.remove_suffix(1);
				if (strLine.size() > 2 && strLine[1] == ' ' && (strLine[0] == 'C' || strLine[0] == 'S' || strLine[0] == 'F'))
				{
					++rSummary.uOwners;
					strOwner = strLine.substr(2);
					const auto itStruct = mapStructs.find(std::string(strOwner));
					pOwner = itStruct == mapStructs.end() ? nullptr : itStruct->second;
					stOffsets = pOwner ? ReflectedOffsets(pOwner) : stReflected{};
					if (!pOwner)
					{
						++rSummary.uOwnersMissing;
						rMissing.push_back(strOwner);
					}
					continue;
				}
				const std::size_t uSpace = strLine.find(' ');
				if (uSpace == std::string_view::npos)
					continue;
				const std::string_view strMember = strLine.substr(0, uSpace);
				const std::string_view strOffset = strLine.substr(uSpace + 1);
				std::size_t uSdkOffset = 0;
				std::from_chars(strOffset.data(), strOffset.data() + strOffset.size(), uSdkOffset, 16);
				++rSummary.uMembers;
				if (!pOwner)
				{
					++rSummary.uOwnerMissing;
					continue;
				}
				const auto itOffset = stOffsets.mapExact.find(std::string(strMember));
				if (itOffset == stOffsets.mapExact.end() && MatchesRenamed(stOffsets, strMember, uSdkOffset))
				{
					++rSummary.uMatched;
					continue;
				}
				if (itOffset == stOffsets.mapExact.end())
				{
					++rSummary.uNotReflected;
					rNotReflected.push_back({ strOwner, strMember, uSdkOffset, 0 });
				}
				else if (itOffset->second != uSdkOffset)
				{
					++rSummary.uMoved;
					rMoved.push_back({ strOwner, strMember, uSdkOffset, itOffset->second });
				}
				else
					++rSummary.uMatched;
			}
		}

		void WriteReport(const stSummary& rSummary, const std::vector<stIssue>& aMoved, const std::vector<stIssue>& aNotReflected, const std::vector<std::string_view>& aMissing)
		{
			std::ofstream fileReport(rSummary.pathReport, std::ios::trunc);
			if (!fileReport)
				return;
			fileReport << std::format("SDK audit for {}\n{} members in {} structs: {} match, {} moved, {} not reflected, {} in {} structs that are not loaded\n\n", kGame, rSummary.uMembers, rSummary.uOwners, rSummary.uMatched, rSummary.uMoved, rSummary.uNotReflected,
				rSummary.uOwnerMissing, rSummary.uOwnersMissing);
			fileReport << std::format("Moved ({}): use OW_FIELD for these or dump a new SDK\n", aMoved.size());
			for (const stIssue& rIssue : aMoved)
				fileReport << std::format("  {}::{} SDK 0x{:X} game 0x{:X}\n", rIssue.strOwner, rIssue.strMember, rIssue.uSdkOffset, rIssue.uGameOffset);
			fileReport << std::format("\nNot reflected ({}): the SDK has the member but the game's reflection data does not\n", aNotReflected.size());
			for (const stIssue& rIssue : aNotReflected)
				fileReport << std::format("  {}::{} SDK 0x{:X}\n", rIssue.strOwner, rIssue.strMember, rIssue.uSdkOffset);
			fileReport << std::format("\nNot loaded ({}): classes, structs and functions that are not in memory right now (blueprints load with their level)\n", aMissing.size());
			for (const std::string_view strOwner : aMissing)
				fileReport << std::format("  {}\n", strOwner);
		}
	}

	stSummary Run()
	{
		std::scoped_lock lckAudit(g_mtxAudit);
		stSummary stResult;
		const std::string_view strTable = EmbeddedTable();
		if (strTable.empty())
		{
			LogWarning("unreal audit: this build has no embedded SDK offset table");
			return stResult;
		}
		if (!Resolver::IsResolved())
		{
			LogWarning("unreal audit: the game's globals are not resolved");
			return stResult;
		}
		const auto tpStart = std::chrono::steady_clock::now();
		std::vector<stIssue> aMoved;
		std::vector<stIssue> aNotReflected;
		std::vector<std::string_view> aMissing;
		OmegaRenderHook::stFault stFault;
		auto fnAudit = [&] { Audit(strTable, stResult, aMoved, aNotReflected, aMissing); };
		if (!OmegaRenderHook::InvokeGuarded(fnAudit, stFault))
		{
			LogError("unreal audit faulted (code 0x{:08X} at {})", stFault.uCode, stFault.pAddress);
			return stResult;
		}
		stResult.bRan = true;
		stResult.llMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - tpStart).count();
		stResult.pathReport = DataDirectory() / "Logs" / "SdkAudit.txt";
		WriteReport(stResult, aMoved, aNotReflected, aMissing);
		const std::string strSummary = std::format("unreal audit: {} SDK members in {} structs: {} match, {} moved, {} not reflected, {} in {} structs that are not loaded ({} ms, report in {})", stResult.uMembers, stResult.uOwners, stResult.uMatched,
			stResult.uMoved, stResult.uNotReflected, stResult.uOwnerMissing, stResult.uOwnersMissing, stResult.llMilliseconds, stResult.pathReport.string());
		if (stResult.uMoved)
			LogWarning("{}; members that moved read wrong memory through '->', use OW_FIELD for them or dump a new SDK", strSummary);
		else
			LogInfo("{}", strSummary);
		return stResult;
	}
}

#endif
