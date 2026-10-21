#include "Engines/Unreal/Resolver.hpp"

#if OMEGAWARE_ENGINE_UNREAL

#include "Core/Log.hpp"
#include "Engines/Unreal/NamePool.hpp"
#include "Engines/Unreal/Unreal.hpp"

#include <Windows.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstring>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace OmegaWare::Unreal::Resolver
{
	namespace
	{
		enum class ESource
		{
			Missing,
			Sdk,
			Scan,
		};

		struct stSection
		{
			std::uintptr_t uStart = 0;
			std::size_t uSize = 0;
		};

		struct stFound
		{
			std::uintptr_t uGObjects = 0;
			ESource eGObjects = ESource::Missing;
			std::uintptr_t uNamePool = 0;
			NamePool::stLayout stNames;
			ESource eNames = ESource::Missing;
			std::uintptr_t uProcessEvent = 0;
			std::uintptr_t uWorldSlot = 0;
			ESource eWorld = ESource::Missing;
			std::size_t uWorldSlots = 0;
		};

		struct stState
		{
			std::mutex mtxResolve;
			bool bAttempted = false;
			std::atomic<bool> bResolved = false;
			std::uintptr_t uBase = 0;
			std::vector<stSection> aData;
			stFound stCurrent;
			std::atomic<std::uintptr_t> uWorldSlot = 0;
			std::chrono::steady_clock::time_point tpNextWorldScan{};
		};

		stState& State()
		{
			static stState s_stState;
			return s_stState;
		}

		std::atomic<std::uintptr_t> g_uNamePool = 0;
		NamePool::stLayout g_stNameLayout;

		[[nodiscard]] bool ProbeCopy(void* pDestination, const void* pSource, std::size_t uSize)
		{
			__try
			{
				std::memcpy(pDestination, pSource, uSize);
				return true;
			}
			__except (EXCEPTION_EXECUTE_HANDLER)
			{
				return false;
			}
		}

		template <typename T>
		[[nodiscard]] bool Probe(std::uintptr_t uAddress, T& rValue)
		{
			return uAddress && ProbeCopy(&rValue, reinterpret_cast<const void*>(uAddress), sizeof(T));
		}

		[[nodiscard]] bool IsPlausiblePointer(std::uintptr_t uValue)
		{
			return uValue >= 0x10000 && uValue < 0x7FFFFFFF0000 && (uValue & 7) == 0;
		}

		[[nodiscard]] std::size_t GuardedDecode(const std::uint8_t* const* ppBlocks, std::uint32_t uBlockCount, std::uint32_t uIndex, const NamePool::stLayout& stPool, wchar_t* pOut, std::size_t uCapacity)
		{
			__try
			{
				return NamePool::Decode(ppBlocks, uBlockCount, uIndex, stPool, pOut, uCapacity);
			}
			__except (EXCEPTION_EXECUTE_HANDLER)
			{
				return 0;
			}
		}

		[[nodiscard]] std::size_t DecodeName(std::uintptr_t uPool, const NamePool::stLayout& stPool, std::uint32_t uIndex, wchar_t* pOut, std::size_t uCapacity)
		{
			std::uint32_t uCurrentBlock = 0;
			if (!Probe(uPool + NamePool::kCurrentBlockOffset, uCurrentBlock) || uCurrentBlock >= NamePool::kMaxBlocks)
				return 0;
			const auto* ppBlocks = reinterpret_cast<const std::uint8_t* const*>(uPool + NamePool::kBlocksOffset);
			return GuardedDecode(ppBlocks, uCurrentBlock + 1, uIndex, stPool, pOut, uCapacity);
		}

		struct stStringData
		{
			wchar_t* pData;
			std::int32_t iNum;
			std::int32_t iMax;
		};

		void AppendString(const SDK::FName* pName, SDK::FString& rOut)
		{
			auto& rString = reinterpret_cast<stStringData&>(rOut);
			if (!pName || !rString.pData || rString.iMax <= 0)
				return;
			std::array<wchar_t, NamePool::kMaxLength + 16> aName{};
			std::size_t uLength = DecodeName(g_uNamePool.load(std::memory_order_acquire), g_stNameLayout, static_cast<std::uint32_t>(pName->GetDisplayIndex()), aName.data(), NamePool::kMaxLength);
			if (!uLength)
			{
				constexpr std::wstring_view strNone = L"None";
				std::ranges::copy(strNone, aName.begin());
				uLength = strNone.size();
			}
			if (pName->Number)
			{
				const std::wstring strNumber = L"_" + std::to_wstring(pName->Number - 1);
				const std::size_t uCopy = std::min(strNumber.size(), aName.size() - uLength);
				std::copy_n(strNumber.begin(), uCopy, aName.begin() + uLength);
				uLength += uCopy;
			}
			const std::int32_t iStart = rString.iNum > 0 ? rString.iNum - 1 : 0;
			const std::int32_t iCopy = std::clamp(static_cast<std::int32_t>(uLength), 0, std::max(0, rString.iMax - iStart - 1));
			std::copy_n(aName.begin(), iCopy, rString.pData + iStart);
			rString.pData[iStart + iCopy] = L'\0';
			rString.iNum = iStart + iCopy + 1;
		}

		[[nodiscard]] std::vector<stSection> DataSections(std::uintptr_t uBase)
		{
			std::vector<stSection> aSections;
			const auto* pDos = reinterpret_cast<const IMAGE_DOS_HEADER*>(uBase);
			if (!uBase || pDos->e_magic != IMAGE_DOS_SIGNATURE)
				return aSections;
			const auto* pNt = reinterpret_cast<const IMAGE_NT_HEADERS*>(uBase + pDos->e_lfanew);
			if (pNt->Signature != IMAGE_NT_SIGNATURE)
				return aSections;
			const IMAGE_SECTION_HEADER* pSection = IMAGE_FIRST_SECTION(pNt);
			for (WORD i = 0; i < pNt->FileHeader.NumberOfSections; ++i, ++pSection)
			{
				if (!(pSection->Characteristics & IMAGE_SCN_MEM_WRITE) || (pSection->Characteristics & IMAGE_SCN_MEM_EXECUTE) || pSection->Misc.VirtualSize < sizeof(void*))
					continue;
				std::uintptr_t uCursor = uBase + pSection->VirtualAddress;
				const std::uintptr_t uEnd = uCursor + (pSection->Misc.VirtualSize & ~std::size_t(7));
				while (uCursor < uEnd)
				{
					MEMORY_BASIC_INFORMATION stInfo{};
					if (!VirtualQuery(reinterpret_cast<const void*>(uCursor), &stInfo, sizeof(stInfo)))
						break;
					const std::uintptr_t uRegionEnd = std::min(uEnd, reinterpret_cast<std::uintptr_t>(stInfo.BaseAddress) + stInfo.RegionSize);
					constexpr DWORD kReadable = PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY;
					if (stInfo.State == MEM_COMMIT && (stInfo.Protect & kReadable) && !(stInfo.Protect & (PAGE_GUARD | PAGE_NOACCESS)))
						aSections.push_back({ uCursor, uRegionEnd - uCursor });
					uCursor = uRegionEnd;
				}
			}
			return aSections;
		}

		template <typename F>
		void ForEachSlot(const std::vector<stSection>& aSections, F&& fnVisit)
		{
			for (const stSection& rSection : aSections)
				for (std::uintptr_t uSlot = rSection.uStart; uSlot + sizeof(std::uintptr_t) <= rSection.uStart + rSection.uSize; uSlot += sizeof(std::uintptr_t))
					if (!fnVisit(uSlot, *reinterpret_cast<const std::uintptr_t*>(uSlot)))
						return;
		}

		template <typename TArray>
		[[nodiscard]] bool ReadObject(const TArray& rArray, std::int32_t iIndex, std::uintptr_t& rObject)
		{
			rObject = 0;
			if (iIndex < 0 || iIndex >= rArray.NumElements)
				return false;
			std::uintptr_t uItem = 0;
			if constexpr (requires { rArray.NumChunks; })
			{
				const std::int32_t iChunk = iIndex / TArray::ElementsPerChunk;
				if (iChunk >= rArray.NumChunks)
					return false;
				std::uintptr_t uChunk = 0;
				if (!Probe(reinterpret_cast<std::uintptr_t>(rArray.Objects) + iChunk * sizeof(void*), uChunk) || !uChunk)
					return false;
				uItem = uChunk + static_cast<std::uintptr_t>(iIndex % TArray::ElementsPerChunk) * sizeof(SDK::FUObjectItem);
			}
			else
				uItem = reinterpret_cast<std::uintptr_t>(rArray.Objects) + static_cast<std::uintptr_t>(iIndex) * sizeof(SDK::FUObjectItem);
			return Probe(uItem + offsetof(SDK::FUObjectItem, Object), rObject);
		}

		[[nodiscard]] bool IsObjectArray(std::uintptr_t uAddress)
		{
			SDK::TUObjectArray stArray{};
			if (!Probe(uAddress, stArray) || stArray.NumElements < 0x1000 || stArray.NumElements > 0x4000000 || stArray.MaxElements < stArray.NumElements || !IsPlausiblePointer(reinterpret_cast<std::uintptr_t>(stArray.Objects)))
				return false;
			int iValid = 0;
			for (std::int32_t i = 0; i < 64; ++i)
			{
				std::uintptr_t uObject = 0;
				if (!ReadObject(stArray, i, uObject) || !IsPlausiblePointer(uObject))
					continue;
				std::int32_t iIndex = -1;
				std::uintptr_t uClass = 0;
				std::uintptr_t uClassClass = 0;
				if (!Probe(uObject + offsetof(SDK::UObject, Index), iIndex) || iIndex != i || !Probe(uObject + offsetof(SDK::UObject, Class), uClass) || !IsPlausiblePointer(uClass) || !Probe(uClass + offsetof(SDK::UObject, Class), uClassClass) || !IsPlausiblePointer(uClassClass))
					continue;
				++iValid;
			}
			return iValid >= 56;
		}

		[[nodiscard]] std::uintptr_t FindObjectArray(const stState& rState, bool bUseSdk, ESource& rSource)
		{
			const std::uintptr_t uSdk = rState.uBase + SDK::Offsets::GObjects;
			if (bUseSdk && IsObjectArray(uSdk))
			{
				rSource = ESource::Sdk;
				return uSdk;
			}
			std::uintptr_t uFound = 0;
			ForEachSlot(rState.aData, [&](std::uintptr_t uSlot, std::uintptr_t uValue)
			{
				if (!IsPlausiblePointer(uValue))
					return true;
				std::int32_t iMax = 0;
				std::int32_t iNum = 0;
				if (!Probe(uSlot + offsetof(SDK::TUObjectArray, MaxElements), iMax) || !Probe(uSlot + offsetof(SDK::TUObjectArray, NumElements), iNum) || iNum < 0x1000 || iMax < iNum || !IsObjectArray(uSlot))
					return true;
				uFound = uSlot;
				return false;
			});
			rSource = uFound ? ESource::Scan : ESource::Missing;
			return uFound;
		}

		[[nodiscard]] std::optional<NamePool::stLayout> PoolLayout(std::uintptr_t uPool)
		{
			std::uintptr_t uFirstBlock = 0;
			std::array<std::uint8_t, 32> aFirst{};
			if (!Probe(uPool + NamePool::kBlocksOffset, uFirstBlock) || !IsPlausiblePointer(uFirstBlock) || !Probe(uFirstBlock, aFirst))
				return std::nullopt;
			return NamePool::DetectLayout(aFirst.data());
		}

		[[nodiscard]] std::uintptr_t FindNamePool(const stState& rState, bool bUseSdk, NamePool::stLayout& rLayout, ESource& rSource)
		{
			const std::uintptr_t uSdk = rState.uBase + SDK::Offsets::GNames;
			if (bUseSdk)
			{
				if (const auto optLayout = PoolLayout(uSdk))
				{
					rLayout = *optLayout;
					rSource = ESource::Sdk;
					return uSdk;
				}
			}
			std::uintptr_t uFound = 0;
			ForEachSlot(rState.aData, [&](std::uintptr_t uSlot, std::uintptr_t uValue)
			{
				if (!IsPlausiblePointer(uValue) || uSlot < rState.uBase + NamePool::kBlocksOffset)
					return true;
				const std::uintptr_t uPool = uSlot - NamePool::kBlocksOffset;
				std::array<std::uint32_t, 2> aCursor{};
				if (!Probe(uPool + NamePool::kCurrentBlockOffset, aCursor) || aCursor[0] >= NamePool::kMaxBlocks || aCursor[1] >= NamePool::kMaxBlockBytes)
					return true;
				std::uintptr_t uLast = 0;
				std::uintptr_t uNext = 0;
				if (!Probe(uSlot + aCursor[0] * sizeof(void*), uLast) || !IsPlausiblePointer(uLast) || !Probe(uSlot + (aCursor[0] + 1) * sizeof(void*), uNext) || uNext)
					return true;
				const auto optLayout = PoolLayout(uPool);
				if (!optLayout)
					return true;
				rLayout = *optLayout;
				uFound = uPool;
				return false;
			});
			rSource = uFound ? ESource::Scan : ESource::Missing;
			return uFound;
		}

		[[nodiscard]] bool NamesWork(std::uintptr_t uGObjects, std::uintptr_t uPool, const NamePool::stLayout& stPool)
		{
			SDK::TUObjectArray stArray{};
			if (!Probe(uGObjects, stArray))
				return false;
			const std::int32_t iLimit = std::min(stArray.NumElements, 4096);
			for (std::int32_t i = 0; i < iLimit; ++i)
			{
				std::uintptr_t uObject = 0;
				SDK::FName stName{};
				std::uintptr_t uClass = 0;
				SDK::FName stClassName{};
				if (!ReadObject(stArray, i, uObject) || !Probe(uObject + offsetof(SDK::UObject, Name), stName) || !Probe(uObject + offsetof(SDK::UObject, Class), uClass) || !Probe(uClass + offsetof(SDK::UObject, Name), stClassName))
					continue;
				std::array<wchar_t, NamePool::kMaxLength> aName{};
				const std::size_t uName = DecodeName(uPool, stPool, static_cast<std::uint32_t>(stName.GetDisplayIndex()), aName.data(), aName.size());
				if (!NamePool::Equals(aName.data(), uName, "Object"))
					continue;
				const std::size_t uClassName = DecodeName(uPool, stPool, static_cast<std::uint32_t>(stClassName.GetDisplayIndex()), aName.data(), aName.size());
				if (NamePool::Equals(aName.data(), uClassName, "Class"))
					return true;
			}
			return false;
		}

		[[nodiscard]] std::uintptr_t FindProcessEvent(std::uintptr_t uGObjects)
		{
			SDK::TUObjectArray stArray{};
			std::uintptr_t uObject = 0;
			std::uintptr_t uTable = 0;
			std::uintptr_t uFunction = 0;
			if (!Probe(uGObjects, stArray) || !ReadObject(stArray, 0, uObject) || !Probe(uObject, uTable) || !IsPlausiblePointer(uTable) || !Probe(uTable + SDK::Offsets::ProcessEventIdx * sizeof(void*), uFunction))
				return 0;
			MEMORY_BASIC_INFORMATION stInfo{};
			if (!VirtualQuery(reinterpret_cast<const void*>(uFunction), &stInfo, sizeof(stInfo)) || stInfo.State != MEM_COMMIT || !(stInfo.Protect & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)))
				return 0;
			return uFunction;
		}

		[[nodiscard]] SDK::UClass* WorldClass()
		{
			static SDK::UClass* s_pClass = nullptr;
			if (!s_pClass)
				s_pClass = SDK::UWorld::StaticClass();
			return s_pClass;
		}

		[[nodiscard]] bool IsLiveWorld(std::uintptr_t uObject)
		{
			std::uintptr_t uClass = 0;
			SDK::EObjectFlags eFlags{};
			const SDK::UClass* pWorldClass = WorldClass();
			return pWorldClass && IsPlausiblePointer(uObject) && Probe(uObject + offsetof(SDK::UObject, Class), uClass) && uClass == reinterpret_cast<std::uintptr_t>(pWorldClass) && Probe(uObject + offsetof(SDK::UObject, Flags), eFlags)
				&& !(static_cast<std::uint32_t>(eFlags) & (static_cast<std::uint32_t>(SDK::EObjectFlags::ClassDefaultObject) | static_cast<std::uint32_t>(SDK::EObjectFlags::ArchetypeObject)));
		}

		[[nodiscard]] std::uintptr_t FindWorldSlot(const stState& rState, std::uintptr_t uGObjects, bool bUseSdk, ESource& rSource, std::size_t& rSlots)
		{
			rSlots = 0;
			const std::uintptr_t uSdk = rState.uBase + SDK::Offsets::GWorld;
			std::uintptr_t uSdkWorld = 0;
			if (bUseSdk && SDK::Offsets::GWorld && Probe(uSdk, uSdkWorld) && IsLiveWorld(uSdkWorld))
			{
				rSource = ESource::Sdk;
				rSlots = 1;
				return uSdk;
			}
			SDK::TUObjectArray stArray{};
			if (!Probe(uGObjects, stArray))
				return 0;
			std::vector<std::uintptr_t> aWorlds;
			for (std::int32_t i = 0; i < stArray.NumElements && aWorlds.size() < 16; ++i)
			{
				std::uintptr_t uObject = 0;
				if (ReadObject(stArray, i, uObject) && IsLiveWorld(uObject))
					aWorlds.push_back(uObject);
			}
			if (aWorlds.empty())
				return 0;
			std::uintptr_t uFound = 0;
			ForEachSlot(rState.aData, [&](std::uintptr_t uSlot, std::uintptr_t uValue)
			{
				if (std::ranges::find(aWorlds, uValue) == aWorlds.end())
					return true;
				++rSlots;
				if (!uFound || uSlot == uSdk)
					uFound = uSlot;
				return true;
			});
			rSource = uFound ? ESource::Scan : ESource::Missing;
			return uFound;
		}

		[[nodiscard]] std::string_view SourceName(ESource eSource)
		{
			switch (eSource)
			{
			case ESource::Sdk: return "SDK offset still valid";
			case ESource::Scan: return "found by scanning, the SDK offset is stale";
			default: return "not found";
			}
		}

		[[nodiscard]] std::uintptr_t Rva(const stState& rState, std::uintptr_t uAddress)
		{
			return uAddress ? uAddress - rState.uBase : 0;
		}

		void LogFound(const stState& rState, const stFound& rFound)
		{
			LogInfo("unreal: GObjects at +0x{:X} ({}, SDK has +0x{:X})", Rva(rState, rFound.uGObjects), SourceName(rFound.eGObjects), static_cast<std::uint32_t>(SDK::Offsets::GObjects));
			LogInfo("unreal: name pool at +0x{:X} ({}, SDK has +0x{:X}, length shift {}, {})", Rva(rState, rFound.uNamePool), SourceName(rFound.eNames), static_cast<std::uint32_t>(SDK::Offsets::GNames), rFound.stNames.iLengthShift, rFound.stNames.CasePreserving() ? "case-preserving entries" : "plain entries");
			LogInfo("unreal: ProcessEvent at +0x{:X} (vtable slot {}, SDK has +0x{:X}{})", Rva(rState, rFound.uProcessEvent), static_cast<int>(SDK::Offsets::ProcessEventIdx), static_cast<std::uint32_t>(SDK::Offsets::ProcessEvent),
				Rva(rState, rFound.uProcessEvent) == static_cast<std::uintptr_t>(SDK::Offsets::ProcessEvent) ? ", matches" : ", differs");
			if (rFound.uWorldSlot)
				LogInfo("unreal: GWorld at +0x{:X} ({}, SDK has +0x{:X}, {} candidate slots)", Rva(rState, rFound.uWorldSlot), SourceName(rFound.eWorld), static_cast<std::uint32_t>(SDK::Offsets::GWorld), rFound.uWorldSlots);
			else
				LogInfo("unreal: GWorld not found yet, it is looked up again until a world exists");
		}

		void InitBase(stState& rState)
		{
			if (rState.uBase)
				return;
			rState.uBase = SDK::InSDKUtils::GetImageBase();
			rState.aData = DataSections(rState.uBase);
		}
	}

	bool Resolve()
	{
		stState& rState = State();
		std::scoped_lock lckResolve(rState.mtxResolve);
		if (rState.bAttempted)
			return rState.bResolved;
		rState.bAttempted = true;
		InitBase(rState);
		if (rState.aData.empty())
		{
			LogError("unreal: the game executable has no writable data sections, cannot find its globals");
			return false;
		}

		const auto tpStart = std::chrono::steady_clock::now();
		stFound& rFound = rState.stCurrent;
		rFound.uGObjects = FindObjectArray(rState, true, rFound.eGObjects);
		if (!rFound.uGObjects)
		{
			LogError("unreal: could not find the game's object array (GObjects), the SDK may be for a different engine version");
			return false;
		}
		rFound.uNamePool = FindNamePool(rState, true, rFound.stNames, rFound.eNames);
		if (!rFound.uNamePool || !NamesWork(rFound.uGObjects, rFound.uNamePool, rFound.stNames))
		{
			LogError("unreal: could not find a working name pool (FNamePool), the SDK may be for a different engine version");
			return false;
		}
		rFound.uProcessEvent = FindProcessEvent(rFound.uGObjects);
		if (!rFound.uProcessEvent)
		{
			LogError("unreal: UObject vtable slot {} is not code, the SDK's ProcessEvent index does not fit this game", static_cast<int>(SDK::Offsets::ProcessEventIdx));
			return false;
		}

		g_stNameLayout = rFound.stNames;
		g_uNamePool.store(rFound.uNamePool, std::memory_order_release);
		SDK::UObject::GObjects.InitManually(reinterpret_cast<void*>(rFound.uGObjects));
		SDK::FName::InitManually(reinterpret_cast<void*>(&AppendString));
		rState.bResolved = true;

		rFound.uWorldSlot = FindWorldSlot(rState, rFound.uGObjects, true, rFound.eWorld, rFound.uWorldSlots);
		rState.uWorldSlot.store(rFound.uWorldSlot, std::memory_order_release);
		LogFound(rState, rFound);
		LogInfo("unreal: globals resolved in {} ms", std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - tpStart).count());
		return true;
	}

	bool IsResolved()
	{
		return State().bResolved.load(std::memory_order_acquire);
	}

	SDK::UWorld* World()
	{
		stState& rState = State();
		if (!IsResolved())
			return nullptr;
		std::uintptr_t uSlot = rState.uWorldSlot.load(std::memory_order_acquire);
		if (!uSlot)
		{
			std::scoped_lock lckResolve(rState.mtxResolve);
			const auto tpNow = std::chrono::steady_clock::now();
			uSlot = rState.uWorldSlot.load(std::memory_order_acquire);
			if (!uSlot && tpNow >= rState.tpNextWorldScan)
			{
				rState.tpNextWorldScan = tpNow + std::chrono::seconds(1);
				stFound& rFound = rState.stCurrent;
				uSlot = FindWorldSlot(rState, rFound.uGObjects, true, rFound.eWorld, rFound.uWorldSlots);
				if (uSlot)
				{
					rFound.uWorldSlot = uSlot;
					rState.uWorldSlot.store(uSlot, std::memory_order_release);
					LogInfo("unreal: GWorld at +0x{:X} ({}, SDK has +0x{:X}, {} candidate slots)", Rva(rState, uSlot), SourceName(rFound.eWorld), static_cast<std::uint32_t>(SDK::Offsets::GWorld), rFound.uWorldSlots);
				}
			}
		}
		std::uintptr_t uWorld = 0;
		if (!uSlot || !Probe(uSlot, uWorld) || !IsLiveWorld(uWorld))
			return nullptr;
		return reinterpret_cast<SDK::UWorld*>(uWorld);
	}

	void* ProcessEventAddress()
	{
		return IsResolved() ? reinterpret_cast<void*>(State().stCurrent.uProcessEvent) : nullptr;
	}

	void LogReport()
	{
		stState& rState = State();
		std::scoped_lock lckResolve(rState.mtxResolve);
		if (!rState.bResolved)
		{
			LogWarning("unreal: globals are not resolved");
			return;
		}
		LogFound(rState, rState.stCurrent);
	}

	bool LogScanTest()
	{
		stState& rState = State();
		std::scoped_lock lckResolve(rState.mtxResolve);
		if (!rState.bResolved)
		{
			LogWarning("unreal scan test: globals are not resolved");
			return false;
		}
		const auto tpStart = std::chrono::steady_clock::now();
		stFound stScan;
		stScan.uGObjects = FindObjectArray(rState, false, stScan.eGObjects);
		stScan.uNamePool = stScan.uGObjects ? FindNamePool(rState, false, stScan.stNames, stScan.eNames) : 0;
		const bool bNames = stScan.uNamePool && NamesWork(stScan.uGObjects, stScan.uNamePool, stScan.stNames);
		stScan.uProcessEvent = stScan.uGObjects ? FindProcessEvent(stScan.uGObjects) : 0;
		stScan.uWorldSlot = stScan.uGObjects ? FindWorldSlot(rState, stScan.uGObjects, false, stScan.eWorld, stScan.uWorldSlots) : 0;
		const auto msElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - tpStart).count();

		const stFound& rCurrent = rState.stCurrent;
		const auto fnCompare = [&](std::string_view strName, std::uintptr_t uScan, std::uintptr_t uSdk)
		{
			const bool bMatch = uScan && uScan == uSdk;
			LogInfo("unreal scan test: {} scan +0x{:X}, SDK +0x{:X} {}", strName, Rva(rState, uScan), Rva(rState, uSdk), bMatch ? "match" : "DIFFER");
			return bMatch;
		};
		bool bPassed = fnCompare("GObjects", stScan.uGObjects, rState.uBase + SDK::Offsets::GObjects);
		bPassed = fnCompare("name pool", stScan.uNamePool, rState.uBase + SDK::Offsets::GNames) && bNames && bPassed;
		bPassed = fnCompare("ProcessEvent", stScan.uProcessEvent, rState.uBase + SDK::Offsets::ProcessEvent) && bPassed;
		bPassed = fnCompare("GWorld", stScan.uWorldSlot, rState.uBase + SDK::Offsets::GWorld) && bPassed;
		LogInfo("unreal scan test: names decode {}, {} GWorld candidate slots, scanned in {} ms, current GObjects +0x{:X}", bNames ? "ok" : "FAILED", stScan.uWorldSlots, msElapsed, Rva(rState, rCurrent.uGObjects));
		LogInfo("unreal scan test {}", bPassed ? "passed" : "FAILED");
		return bPassed;
	}
}

#endif
