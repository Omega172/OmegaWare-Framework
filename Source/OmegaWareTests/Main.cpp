#include "Core/CrashDump.hpp"
#include "Core/Log.hpp"
#if OMEGAWARE_ENGINE_UNITY
#include "Engines/Unity/Il2Cpp.hpp"
#include "Engines/Unity/Mono.hpp"
#endif
#include "Engines/Unreal/NamePool.hpp"
#include "Hooks/HookBatch.hpp"
#include "Memory/Memory.hpp"
#include "OmegaRenderHook/Core/Guarded.hpp"
#include "OmegaRenderHook/Hook/InlineHook.hpp"
#include "OmegaRenderHook/Input/InputBlock.hpp"
#include "OmegaRenderHook/Render/Pipeline.hpp"
#include "OmegaRenderHook/Window/WindowHook.hpp"

#include <Windows.h>
#include <winver.h>
#include <Xinput.h>

#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <GameInput.h>

#include <algorithm>
#include <array>
#include <cstdio>
#include <filesystem>
#include <vector>
#include <cstring>
#include <format>
#include <fstream>
#include <string>

namespace
{
	int g_iFailures = 0;
	int g_iChecks = 0;

	void Check(std::string_view strName, bool bPassed, std::string_view strDetail = {})
	{
		++g_iChecks;
		if (!bPassed)
			++g_iFailures;
		std::printf("  [%s] %.*s%s%.*s\n", bPassed ? "pass" : "FAIL", static_cast<int>(strName.size()), strName.data(), strDetail.empty() || bPassed ? "" : " - ",
			bPassed ? 0 : static_cast<int>(strDetail.size()), strDetail.data());
	}

	alignas(16) const unsigned char g_aMarker[] ={ 0x4F, 0x6D, 0x65, 0x67, 0x61, 0xA5, 0x5A, 0xC3, 0x3C, 0x91, 0x19, 0xE7, 0x7E, 0x00, 0xFF, 0x42 };

	__declspec(noinline) int TargetAdd(int iLeft, int iRight)
	{
		return iLeft + iRight + static_cast<int>(g_aMarker[15] & 0);
	}

	__declspec(noinline) int TargetSubtract(int iLeft, int iRight)
	{
		return iLeft - iRight + static_cast<int>(g_aMarker[14] & 0);
	}

	using FnBinary = int (*)(int, int);
	int (*volatile g_pfnAdd)(int, int) = &TargetAdd;
	int (*volatile g_pfnSubtract)(int, int) = &TargetSubtract;

	OmegaWare::Hooks::clHook<FnBinary> g_clAddHook("test.add");
	OmegaWare::Hooks::clHook<FnBinary> g_clSubtractHook("test.subtract");

	int DetourAdd(int iLeft, int iRight)
	{
		OmegaWare::Hooks::clCallGuard clGuard;
		return g_clAddHook(iLeft, iRight) * 10;
	}

	int DetourSubtract(int iLeft, int iRight)
	{
		OmegaWare::Hooks::clCallGuard clGuard;
		return g_clSubtractHook(iLeft, iRight) * 100;
	}

	struct clVirtualBase
	{
		virtual ~clVirtualBase() = default;
		virtual int Value() { return 7; }
		virtual int Other() { return 9; }
	};

	void TestPatterns()
	{
		std::puts("patterns");
		constexpr auto aPattern = OmegaWare::Memory::Pattern<"48 8B ? ?? 05">;
		Check("compile-time pattern has five tokens", aPattern.size() == 5);
		Check("compile-time pattern bytes and wildcards", aPattern[0] == 0x48 && aPattern[1] == 0x8B && aPattern[2] == OmegaWare::Memory::kWildcard && aPattern[3] == OmegaWare::Memory::kWildcard && aPattern[4] == 0x05);
		const auto optParsed = OmegaWare::Memory::ParsePattern("  de ad ?  BE ef ");
		Check("runtime pattern parses mixed case and spacing", optParsed && optParsed->size() == 5 && (*optParsed)[0] == 0xDE && (*optParsed)[2] == OmegaWare::Memory::kWildcard && (*optParsed)[4] == 0xEF);
		Check("runtime pattern rejects bad tokens", !OmegaWare::Memory::ParsePattern("48 8G") && !OmegaWare::Memory::ParsePattern("488B") && !OmegaWare::Memory::ParsePattern(""));
	}

	void TestScanning()
	{
		std::puts("scanning");
		unsigned char aBuffer[64]{};
		for (int i = 0; i < 64; ++i)
			aBuffer[i] = static_cast<unsigned char>(i);
		const auto uBuffer = reinterpret_cast<std::uintptr_t>(aBuffer);
		constexpr auto aMiddle = OmegaWare::Memory::Pattern<"10 ? 12 13">;
		Check("finds pattern with wildcard", OmegaWare::Memory::FindPattern(uBuffer, sizeof(aBuffer), aMiddle) == uBuffer + 0x10);
		constexpr auto aTail = OmegaWare::Memory::Pattern<"3D 3E 3F">;
		Check("finds pattern ending on the last byte", OmegaWare::Memory::FindPattern(uBuffer, sizeof(aBuffer), aTail) == uBuffer + 0x3D);
		constexpr auto aLeading = OmegaWare::Memory::Pattern<"? ? 02 03">;
		Check("finds pattern with leading wildcards", OmegaWare::Memory::FindPattern(uBuffer, sizeof(aBuffer), aLeading) == uBuffer);
		constexpr auto aMissing = OmegaWare::Memory::Pattern<"3F 40">;
		Check("returns 0 when the pattern is missing", OmegaWare::Memory::FindPattern(uBuffer, sizeof(aBuffer), aMissing) == 0);

		unsigned char aRepeated[32]{};
		for (int i = 0; i < 32; i += 8)
			aRepeated[i] = 0xAB, aRepeated[i + 1] = 0xCD;
		constexpr auto aPair = OmegaWare::Memory::Pattern<"AB CD">;
		Check("finds every match", OmegaWare::Memory::FindAllPatterns(reinterpret_cast<std::uintptr_t>(aRepeated), sizeof(aRepeated), aPair).size() == 4);
		Check("respects the match limit", OmegaWare::Memory::FindAllPatterns(reinterpret_cast<std::uintptr_t>(aRepeated), sizeof(aRepeated), aPair, 2).size() == 2);

		SYSTEM_INFO stSystem{};
		GetSystemInfo(&stSystem);
		const std::size_t uPage = stSystem.dwPageSize;
		auto* pPages = static_cast<unsigned char*>(VirtualAlloc(nullptr, uPage * 3, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
		std::memset(pPages, 0x90, uPage * 3);
		const unsigned char aNeedle[] = { 0xC0, 0xFF, 0xEE, 0x11 };
		std::memcpy(pPages + uPage * 2 + 100, aNeedle, sizeof(aNeedle));
		DWORD uOld = 0;
		VirtualProtect(pPages + uPage, uPage, PAGE_NOACCESS, &uOld);
		constexpr auto aCoffee = OmegaWare::Memory::Pattern<"C0 FF EE 11">;
		Check("skips a no-access page and finds the match after it", OmegaWare::Memory::FindPattern(reinterpret_cast<std::uintptr_t>(pPages), uPage * 3, aCoffee) == reinterpret_cast<std::uintptr_t>(pPages + uPage * 2 + 100));
		VirtualFree(pPages, 0, MEM_RELEASE);

		const auto optMain = OmegaWare::Memory::GetModule();
		Check("main module is described", optMain && optMain->uBase && optMain->uSize && optMain->strName == "OmegaWareTests.exe", optMain ? optMain->strName : "none");
		constexpr auto aMarker = OmegaWare::Memory::Pattern<"4F 6D 65 67 61 A5 5A C3 3C 91 19 E7 7E 00 FF 42">;
		Check("finds a marker inside the main module", optMain && OmegaWare::Memory::FindPattern(*optMain, aMarker) == reinterpret_cast<std::uintptr_t>(g_aMarker));
		Check("finds kernel32 by name", OmegaWare::Memory::GetModule("kernel32.dll").has_value());
		Check("unloaded module is reported missing", !OmegaWare::Memory::GetModule("definitely_not_loaded.dll").has_value());
		const auto aModules = OmegaWare::Memory::GetModules();
		Check("module list includes the main module", std::ranges::any_of(aModules, [&](const auto& rModule) { return optMain && rModule.uBase == optMain->uBase; }));

		const OmegaWare::Memory::stSignature aSignatures[] = {
			{ aMissing, {} },
			{ aMiddle, [](std::uintptr_t uAddress) { return uAddress + 1; } },
		};
		const OmegaWare::Memory::stModule stBuffer{ "buffer", nullptr, uBuffer, sizeof(aBuffer) };
		Check("first signature falls through to the next and resolves", OmegaWare::Memory::FindFirst(stBuffer, aSignatures) == uBuffer + 0x11);

		const unsigned char aCall[] = { 0xE8, 0x10, 0x00, 0x00, 0x00, 0xE8, 0xFB, 0xFF, 0xFF, 0xFF };
		const auto uCall = reinterpret_cast<std::uintptr_t>(aCall);
		Check("resolves a forward rel32", OmegaWare::Memory::ResolveRelative(uCall, 1, 5) == uCall + 5 + 0x10);
		Check("resolves a backward rel32", OmegaWare::Memory::ResolveRelative(uCall + 5, 1, 5) == uCall + 5);
	}

	std::size_t AddNameEntry(std::vector<std::uint8_t>& rBlock, std::size_t uOffset, std::u16string_view strText, bool bWide, int iShift)
	{
		const auto uHeader = static_cast<std::uint16_t>((strText.size() << iShift) | (bWide ? 1 : 0));
		std::memcpy(rBlock.data() + uOffset, &uHeader, sizeof(uHeader));
		uOffset += sizeof(uHeader);
		for (const char16_t chValue : strText)
		{
			if (bWide)
			{
				std::memcpy(rBlock.data() + uOffset, &chValue, sizeof(chValue));
				uOffset += sizeof(chValue);
			}
			else
				rBlock[uOffset++] = static_cast<std::uint8_t>(chValue);
		}
		return (uOffset + 1) & ~std::size_t(1);
	}

	std::size_t AddCasePreservingEntry(std::vector<std::uint8_t>& rBlock, std::size_t uOffset, std::uint32_t uComparisonId, std::u16string_view strText, bool bWide)
	{
		std::memcpy(rBlock.data() + uOffset, &uComparisonId, sizeof(uComparisonId));
		const std::size_t uEnd = AddNameEntry(rBlock, uOffset + sizeof(uComparisonId), strText, bWide, 6);
		return (uEnd + 3) & ~std::size_t(3);
	}

	void TestNamePool()
	{
		namespace NamePool = OmegaWare::Unreal::NamePool;
		std::puts("unreal name pool");
		std::vector<std::uint8_t> aFirst(NamePool::kBlockBytes);
		std::size_t uOffset = AddNameEntry(aFirst, 0, u"None", false, 6);
		const std::size_t uByteProperty = uOffset;
		uOffset = AddNameEntry(aFirst, uOffset, u"ByteProperty", false, 6);
		const std::size_t uWide = uOffset;
		uOffset = AddNameEntry(aFirst, uOffset, u"Пес", true, 6);
		const std::size_t uOdd = uOffset;
		AddNameEntry(aFirst, uOffset, u"Pawn", false, 6);
		std::vector<std::uint8_t> aSecond(NamePool::kBlockBytes);
		AddNameEntry(aSecond, 0x40, u"PlayerController", false, 6);
		const std::uint8_t* aBlocks[2] = { aFirst.data(), aSecond.data() };

		const auto optLayout = NamePool::DetectLayout(aFirst.data());
		Check("detects the UE5 name entry header", optLayout.has_value() && optLayout->iLengthShift == 6);
		const NamePool::stLayout stPool = optLayout.value_or(NamePool::stLayout{});
		wchar_t aName[NamePool::kMaxLength]{};
		std::size_t uLength = NamePool::Decode(aBlocks, 2, static_cast<std::uint32_t>(uByteProperty / 2), stPool, aName, std::size(aName));
		Check("decodes an ANSI name", NamePool::Equals(aName, uLength, "ByteProperty"));
		uLength = NamePool::Decode(aBlocks, 2, static_cast<std::uint32_t>(uWide / 2), stPool, aName, std::size(aName));
		Check("decodes a wide name", uLength == 3 && aName[0] == 0x041F && aName[2] == 0x0441);
		uLength = NamePool::Decode(aBlocks, 2, static_cast<std::uint32_t>(uOdd / 2), stPool, aName, std::size(aName));
		Check("decodes a name after an odd-length entry", NamePool::Equals(aName, uLength, "Pawn"));
		uLength = NamePool::Decode(aBlocks, 2, (1u << 16) | (0x40 / 2), stPool, aName, std::size(aName));
		Check("decodes a name in the second block", NamePool::Equals(aName, uLength, "PlayerController"));
		Check("rejects a block past the current one", NamePool::Decode(aBlocks, 1, (1u << 16) | (0x40 / 2), stPool, aName, std::size(aName)) == 0);
		Check("rejects an empty entry", NamePool::Decode(aBlocks, 2, 0x1000, stPool, aName, std::size(aName)) == 0);
		Check("rejects a name longer than the buffer", NamePool::Decode(aBlocks, 2, static_cast<std::uint32_t>(uByteProperty / 2), stPool, aName, 4) == 0);

		std::vector<std::uint8_t> aOld(NamePool::kBlockBytes);
		AddNameEntry(aOld, AddNameEntry(aOld, 0, u"None", false, 1), u"ByteProperty", false, 1);
		const auto optOld = NamePool::DetectLayout(aOld.data());
		Check("detects the older 15-bit length header", optOld.has_value() && optOld->iLengthShift == 1);
		std::vector<std::uint8_t> aWrong(NamePool::kBlockBytes);
		AddNameEntry(aWrong, AddNameEntry(aWrong, 0, u"Nope", false, 6), u"ByteProperty", false, 6);
		Check("rejects a block that does not start with None", !NamePool::DetectLayout(aWrong.data()));
		Check("compares names case-sensitively", !NamePool::Equals(L"object", 6, "Object") && NamePool::Equals(L"Object", 6, "Object"));

		std::vector<std::uint8_t> aCased(NamePool::kMaxBlockBytes);
		std::size_t uCased = AddCasePreservingEntry(aCased, 0, 0, u"None", false);
		const std::size_t uCasedByteProperty = uCased;
		uCased = AddCasePreservingEntry(aCased, uCased, 1, u"ByteProperty", false);
		const std::size_t uCasedWide = uCased;
		uCased = AddCasePreservingEntry(aCased, uCased, 2, u"Пес", true);
		const std::size_t uCasedLast = uCased;
		AddCasePreservingEntry(aCased, uCased, 3, u"PlayerController", false);
		const std::size_t uCasedHigh = 0x30000;
		AddCasePreservingEntry(aCased, uCasedHigh, 4, u"Pawn", false);
		const std::uint8_t* aCasedBlocks[1] = { aCased.data() };
		const auto optCased = NamePool::DetectLayout(aCased.data());
		Check("detects case-preserving name entries", optCased.has_value() && optCased->CasePreserving() && optCased->uStride == NamePool::kCasePreservingStride);
		const NamePool::stLayout stCased = optCased.value_or(NamePool::stLayout{});
		uLength = NamePool::Decode(aCasedBlocks, 1, static_cast<std::uint32_t>(uCasedByteProperty / 4), stCased, aName, std::size(aName));
		Check("decodes a case-preserving ANSI name", NamePool::Equals(aName, uLength, "ByteProperty"));
		uLength = NamePool::Decode(aCasedBlocks, 1, static_cast<std::uint32_t>(uCasedWide / 4), stCased, aName, std::size(aName));
		Check("decodes a case-preserving wide name", uLength == 3 && aName[0] == 0x041F && aName[2] == 0x0441);
		uLength = NamePool::Decode(aCasedBlocks, 1, static_cast<std::uint32_t>(uCasedLast / 4), stCased, aName, std::size(aName));
		Check("decodes a case-preserving name after a wide entry", NamePool::Equals(aName, uLength, "PlayerController"));
		uLength = NamePool::Decode(aCasedBlocks, 1, static_cast<std::uint32_t>(uCasedHigh / 4), stCased, aName, std::size(aName));
		Check("decodes a case-preserving name past the plain block size", NamePool::Equals(aName, uLength, "Pawn"));
		Check("plain layout is not mistaken for case-preserving", optLayout.has_value() && !optLayout->CasePreserving() && optLayout->uStride == NamePool::kStride);
	}

	void TestSafety()
	{
		std::puts("pointer safety");
		int iLocal = 5;
		Check("stack memory is readable", OmegaWare::Memory::IsReadable(&iLocal, sizeof(iLocal)));
		Check("null is not readable", !OmegaWare::Memory::IsReadable(nullptr, 1));
		Check("code is executable", OmegaWare::Memory::IsExecutable(reinterpret_cast<const void*>(&TargetAdd)));
		Check("stack is not executable", !OmegaWare::Memory::IsExecutable(&iLocal));

		SYSTEM_INFO stSystem{};
		GetSystemInfo(&stSystem);
		const std::size_t uPage = stSystem.dwPageSize;
		auto* pPages = static_cast<unsigned char*>(VirtualAlloc(nullptr, uPage * 2, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
		DWORD uOld = 0;
		VirtualProtect(pPages + uPage, uPage, PAGE_NOACCESS, &uOld);
		Check("range inside a readable page passes", OmegaWare::Memory::IsReadable(pPages, uPage));
		Check("range crossing into a no-access page fails", !OmegaWare::Memory::IsReadable(pPages + uPage - 8, 16));
		Check("read-only page is not writable", (VirtualProtect(pPages, uPage, PAGE_READONLY, &uOld), !OmegaWare::Memory::IsWritable(pPages, 4)));

		Check("safe read of valid memory", OmegaWare::Memory::SafeRead<int>(reinterpret_cast<std::uintptr_t>(&iLocal)) == 5);
		Check("safe read of no-access memory returns nothing", !OmegaWare::Memory::SafeRead<int>(reinterpret_cast<std::uintptr_t>(pPages + uPage)));
		Check("safe read of null returns nothing", !OmegaWare::Memory::SafeRead<int>(0));

		VirtualProtect(pPages, uPage, PAGE_READWRITE, &uOld);
		std::memset(pPages + uPage - 4, 'a', 4);
		Check("safe strlen stops at an unreadable page", OmegaWare::Memory::SafeStrlen(reinterpret_cast<const char*>(pPages + uPage - 4)) == 0);
		Check("safe strlen measures a normal string", OmegaWare::Memory::SafeStrlen("omegaware") == 9);
		Check("safe wcslen measures a wide string", OmegaWare::Memory::SafeWcslen(L"omega") == 5);
		Check("safe strlen respects the limit", OmegaWare::Memory::SafeStrlen("omegaware", 4) == 4);
		VirtualFree(pPages, 0, MEM_RELEASE);

		clVirtualBase clObject;
		Check("virtual method count covers the declared methods", OmegaWare::Memory::GetVirtualMethodCount(&clObject) >= 3);
		Check("virtual method lookup returns executable code", OmegaWare::Memory::IsExecutable(OmegaWare::Memory::GetVirtualMethod(&clObject, 1)));
		Check("virtual method lookup on null returns null", OmegaWare::Memory::GetVirtualMethod(nullptr, 0) == nullptr);
	}

	int g_iInterfaceA = 1;
	int g_iInterfaceB = 2;
	void* CreateInterfaceA() { return &g_iInterfaceA; }
	void* CreateInterfaceB() { return &g_iInterfaceB; }
	OmegaWare::Memory::stInterfaceRegistry g_stRegistryB{ &CreateInterfaceB, "OtherThing002", nullptr };
	OmegaWare::Memory::stInterfaceRegistry g_stRegistryA{ &CreateInterfaceA, "TestInterface001", &g_stRegistryB };
	OmegaWare::Memory::stInterfaceRegistry* volatile g_pInterfaceRegistry = &g_stRegistryA;

	void TestInterfaces()
	{
		std::puts("interfaces");
		wchar_t szExe[MAX_PATH]{};
		GetModuleFileNameW(nullptr, szExe, MAX_PATH);
		const std::string strExe = std::filesystem::path(szExe).filename().string();

		Check("CreateInterface calls the export for an exact name", OmegaWare::Memory::CreateInterface(strExe, "TestInterface001") == &g_iInterfaceA);
		Check("CreateInterface finds a partial name through the registry", OmegaWare::Memory::CreateInterface(strExe, "OtherThing") == &g_iInterfaceB);
		Check("CreateInterface returns nothing for an unknown name", OmegaWare::Memory::CreateInterface(strExe, "Missing999") == nullptr);

		auto* pCode = static_cast<std::uint8_t*>(VirtualAlloc(nullptr, 64, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
		Check("thunk test page allocated", pCode != nullptr);
		if (!pCode)
			return;
		const std::uint8_t aThunk[] = { 0x55, 0x8B, 0xEC, 0x5D, 0xE9 };
		std::memcpy(pCode, aThunk, sizeof(aThunk));
		std::uint8_t* pBody = pCode + 32;
		const std::int32_t iJump = static_cast<std::int32_t>(pBody - (pCode + 9));
		std::memcpy(pCode + 5, &iJump, sizeof(iJump));
		pBody[0] = 0x55;
#ifdef _WIN64
		const std::uint8_t aLoad[] = { 0x48, 0x8B, 0x05 };
		std::memcpy(pBody + 1, aLoad, sizeof(aLoad));
		OmegaWare::Memory::stInterfaceRegistry* pHead = g_pInterfaceRegistry;
		std::memcpy(pCode + 48, &pHead, sizeof(pHead));
		const std::int32_t iRelative = static_cast<std::int32_t>((pCode + 48) - (pBody + 8));
		std::memcpy(pBody + 4, &iRelative, sizeof(iRelative));
#else
		const std::uint8_t aLoad[] = { 0x8B, 0x35 };
		std::memcpy(pBody + 1, aLoad, sizeof(aLoad));
		const auto uAbsolute = static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(&g_pInterfaceRegistry));
		std::memcpy(pBody + 3, &uAbsolute, sizeof(uAbsolute));
#endif
		pBody[8] = 0xC3;
		std::vector<std::string> aNames;
		OmegaWare::Memory::EnumerateInterfaces(reinterpret_cast<std::uintptr_t>(pCode), [&](OmegaWare::Memory::stInterfaceRegistry& rRegistry) {
			aNames.emplace_back(rRegistry.szName);
			return false;
		});
		Check("registry is found behind a push/mov/pop/jmp thunk", aNames == std::vector<std::string>{ "TestInterface001", "OtherThing002" }, std::format("{} entries", aNames.size()));
		VirtualFree(pCode, 0, MEM_RELEASE);
	}

	void TestHooks()
	{
		std::puts("hooks");
		Check("original result before hooking", g_pfnAdd(1, 2) == 3);
		Check("hook installs", g_clAddHook.Install(reinterpret_cast<void*>(&TargetAdd), &DetourAdd));
		Check("detour runs and calls the original", g_pfnAdd(1, 2) == 30, std::to_string(g_pfnAdd(1, 2)));
		Check("calling the hook object reaches the original", g_clAddHook(4, 5) == 9);
		Check("second install is refused", !g_clAddHook.Install(reinterpret_cast<void*>(&TargetAdd), &DetourAdd));
		Check("hook removes", g_clAddHook.Remove() && g_clAddHook.GetStatus() == OmegaWare::Hooks::EHookStatus::Uninstalled);
		Check("original result after removing", g_pfnAdd(1, 2) == 3);

		OmegaWare::Hooks::clHook<FnBinary> clBad("test.bad");
		int iData = 0;
		Check("non-executable target fails cleanly", !clBad.Install(&iData, &DetourAdd) && clBad.GetStatus() == OmegaWare::Hooks::EHookStatus::Failed);

		{
			OmegaWare::Hooks::clHookBatch clBatch;
			clBatch.Install(g_clAddHook, reinterpret_cast<void*>(&TargetAdd), &DetourAdd);
			clBatch.Install(g_clSubtractHook, reinterpret_cast<void*>(&TargetSubtract), &DetourSubtract);
			clBatch.Install(clBad, static_cast<void*>(nullptr), &DetourAdd);
			Check("batch commit fails when one hook fails", !clBatch.Commit());
		}
		Check("failed batch rolled back every hook", !g_clAddHook.IsInstalled() && !g_clSubtractHook.IsInstalled() && g_pfnAdd(2, 2) == 4 && g_pfnSubtract(5, 2) == 3);

		{
			OmegaWare::Hooks::clHookBatch clBatch;
			clBatch.Install(g_clAddHook, reinterpret_cast<void*>(&TargetAdd), &DetourAdd);
			clBatch.Install(g_clSubtractHook, reinterpret_cast<void*>(&TargetSubtract), &DetourSubtract);
			Check("batch commit succeeds", clBatch.Commit());
		}
		Check("committed batch keeps hooks installed", g_pfnAdd(2, 2) == 40 && g_pfnSubtract(5, 2) == 300);
		g_clAddHook.Remove();
		g_clSubtractHook.Remove();

		{
			OmegaWare::Hooks::clHookBatch clBatch;
			clBatch.Install(g_clAddHook, reinterpret_cast<void*>(&TargetAdd), &DetourAdd);
		}
		Check("uncommitted batch rolls back on destruction", !g_clAddHook.IsInstalled() && g_pfnAdd(1, 1) == 2);

		clVirtualBase clObject;
		using FnValue = int (*)(clVirtualBase*);
		static OmegaWare::Hooks::clHook<FnValue> s_clVirtualHook("test.virtual");
		Check("virtual install resolves the vtable slot", s_clVirtualHook.InstallVirtual(&clObject, 1, [](clVirtualBase*) { return 70; }) && s_clVirtualHook.GetTarget() == OmegaWare::Memory::GetVirtualMethod(&clObject, 1));
		s_clVirtualHook.Remove();
	}

	struct stExport
	{
		WORD uOrdinal = 0;
		std::string strName;
	};

	[[nodiscard]] std::vector<stExport> ReadExports(HMODULE hModule)
	{
		const auto pBase = reinterpret_cast<const std::byte*>(hModule);
		const auto pDos = reinterpret_cast<const IMAGE_DOS_HEADER*>(pBase);
		const auto pNt = reinterpret_cast<const IMAGE_NT_HEADERS*>(pBase + pDos->e_lfanew);
		const IMAGE_DATA_DIRECTORY& rDirectory = pNt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
		if (!rDirectory.VirtualAddress)
			return {};
		const auto pExports = reinterpret_cast<const IMAGE_EXPORT_DIRECTORY*>(pBase + rDirectory.VirtualAddress);
		const auto pFunctions = reinterpret_cast<const DWORD*>(pBase + pExports->AddressOfFunctions);
		const auto pNames = reinterpret_cast<const DWORD*>(pBase + pExports->AddressOfNames);
		const auto pOrdinals = reinterpret_cast<const WORD*>(pBase + pExports->AddressOfNameOrdinals);
		std::vector<stExport> aExports;
		for (DWORD uIndex = 0; uIndex < pExports->NumberOfFunctions; ++uIndex)
		{
			if (!pFunctions[uIndex])
				continue;
			stExport stEntry{ static_cast<WORD>(pExports->Base + uIndex) };
			for (DWORD uName = 0; uName < pExports->NumberOfNames; ++uName)
				if (pOrdinals[uName] == uIndex)
					stEntry.strName = reinterpret_cast<const char*>(pBase + pNames[uName]);
			aExports.push_back(std::move(stEntry));
		}
		return aExports;
	}

	void TestVersionProxy(HMODULE hProxy);

	void TestLogFiles()
	{
		std::puts("log files");
		std::error_code errCode;
		const std::filesystem::path pathLogs = std::filesystem::temp_directory_path(errCode) / std::format("OmegaWareTests-{}", GetCurrentProcessId());
		std::filesystem::remove_all(pathLogs, errCode);
		OmegaWare::OpenLog(pathLogs);
		OmegaWare::LogInfo("plain info line");
		OmegaWare::LogWarning("warning line {}", 7);
		OmegaWare::LogError("error line {}", 42);
		OmegaWare::CloseLog();

		const auto fnRead = [](const std::filesystem::path& rPath) { std::ifstream file(rPath); return std::string(std::istreambuf_iterator<char>(file), {}); };
		std::string strLog;
		std::string strErrors;
		std::size_t uFiles = 0;
		for (const auto& rEntry : std::filesystem::directory_iterator(pathLogs, errCode))
		{
			++uFiles;
			const std::string strName = rEntry.path().filename().string();
			(strName.ends_with("_ERRORS.log") ? strErrors : strLog) = fnRead(rEntry.path());
		}
		Check("one log and one error log per launch", uFiles == 2, std::format("{} files", uFiles));
		Check("full log has every level", strLog.find("plain info line") != std::string::npos && strLog.find("warning line 7") != std::string::npos && strLog.find("error line 42") != std::string::npos);
		Check("error log has only errors", strErrors.find("error line 42") != std::string::npos && strErrors.find("warning line") == std::string::npos && strErrors.find("plain info") == std::string::npos, strErrors);
		Check("errors carry file, line, column and function", strErrors.find("[Main.cpp (") != std::string::npos && strErrors.find("TestLogFiles |") != std::string::npos, strErrors);
		Check("info lines carry no location", strLog.find("] plain info line") != std::string::npos);
		std::filesystem::remove_all(pathLogs, errCode);
	}

	void TestCrashDump()
	{
		std::puts("crash dumps");
		wchar_t szTemp[MAX_PATH]{};
		GetTempPathW(MAX_PATH, szTemp);
		const std::filesystem::path pathDumps = std::filesystem::path(szTemp) / std::format(L"OmegaWareTests-{}", GetCurrentProcessId());
		std::error_code ecRemove;
		std::filesystem::remove_all(pathDumps, ecRemove);

		const auto CountDumps = [&] {
			std::error_code ecList;
			std::size_t uCount = 0;
			for (const auto& rEntry : std::filesystem::directory_iterator(pathDumps, ecList))
				uCount += rEntry.path().extension() == ".dmp" ? 1 : 0;
			return uCount;
		};
		const auto Fault = [] {
			auto fnFault = [] { *static_cast<volatile int*>(nullptr) = 1; };
			OmegaRenderHook::stFault stFault;
			return !OmegaRenderHook::InvokeGuarded(fnFault, stFault) && stFault.uCode == EXCEPTION_ACCESS_VIOLATION;
		};

		OmegaWare::CrashDump::Install(pathDumps);
		Check("guarded fault is caught while dumps are enabled", Fault());
		const std::size_t uAfterFault = CountDumps();
		Check("a fault writes one crash dump", uAfterFault == 1, std::format("{} dumps", uAfterFault));
		OmegaWare::CrashDump::Uninstall();
		Check("guarded fault is caught after dumps are disabled", Fault());
		Check("no dump is written after uninstall", CountDumps() == uAfterFault);
		std::filesystem::remove_all(pathDumps, ecRemove);
	}

#if OMEGAWARE_ENGINE_UNITY
	void TestIl2Cpp()
	{
		std::puts("il2cpp");
		Check("initialize fails cleanly without an il2cpp runtime", !OmegaWare::Il2Cpp::Initialize() && !OmegaWare::Il2Cpp::IsAvailable());
		Check("lookups return nothing while unavailable", !OmegaWare::Il2Cpp::GetImage() && !OmegaWare::Il2Cpp::GetClass("", "GameManager") && !OmegaWare::Il2Cpp::GetCompiledMethod("", "GameManager", "Update"));
		Check("static field read returns nothing while unavailable", !OmegaWare::Il2Cpp::GetStaticField<int>(nullptr, "value").has_value());
		Check("string helpers are safe while unavailable", OmegaWare::Il2Cpp::ToString(nullptr).empty() && !OmegaWare::Il2Cpp::NewString("omegaware"));
	}
#endif

	class clForeignCall
	{
	public:
		clForeignCall() { m_pCode = static_cast<std::uint8_t*>(VirtualAlloc(nullptr, uCodeSize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE)); }
		~clForeignCall() { if (m_pCode) VirtualFree(m_pCode, 0, MEM_RELEASE); }

		template <typename... Args>
		[[nodiscard]] std::uintptr_t Call(void* pFunction, Args... args)
		{
			static_assert(sizeof...(Args) <= 6);
			const std::array<std::uintptr_t, sizeof...(Args)> aArguments = { ToArgument(args)... };
			std::uint8_t* p = m_pCode;
			const auto Emit = [&](std::initializer_list<std::uint8_t> aBytes) { for (std::uint8_t by : aBytes) *p++ = by; };
			const auto EmitValue = [&](auto value) { std::memcpy(p, &value, sizeof(value)); p += sizeof(value); };
#ifdef _WIN64
			constexpr std::array<std::array<std::uint8_t, 2>, 4> aRegisters = { { { 0x48, 0xB9 }, { 0x48, 0xBA }, { 0x49, 0xB8 }, { 0x49, 0xB9 } } };
			Emit({ 0x48, 0x83, 0xEC, 0x48 });
			for (std::size_t i = 0; i < aArguments.size(); ++i)
			{
				if (i < aRegisters.size())
				{
					Emit({ aRegisters[i][0], aRegisters[i][1] });
					EmitValue(static_cast<std::uint64_t>(aArguments[i]));
					continue;
				}
				Emit({ 0x48, 0xB8 });
				EmitValue(static_cast<std::uint64_t>(aArguments[i]));
				Emit({ 0x48, 0x89, 0x44, 0x24, static_cast<std::uint8_t>(0x20 + 8 * (i - aRegisters.size())) });
			}
			Emit({ 0x48, 0xB8 });
			EmitValue(reinterpret_cast<std::uint64_t>(pFunction));
			Emit({ 0xFF, 0xD0, 0x48, 0x83, 0xC4, 0x48, 0xC3 });
#else
			for (std::size_t i = aArguments.size(); i-- > 0;)
			{
				Emit({ 0x68 });
				EmitValue(static_cast<std::uint32_t>(aArguments[i]));
			}
			Emit({ 0xB8 });
			EmitValue(reinterpret_cast<std::uint32_t>(pFunction));
			Emit({ 0xFF, 0xD0, 0xC3 });
#endif
			FlushInstructionCache(GetCurrentProcess(), m_pCode, uCodeSize);
			return reinterpret_cast<std::uintptr_t (*)()>(m_pCode)();
		}

		[[nodiscard]] bool IsValid() const { return m_pCode != nullptr; }

	private:
		static constexpr std::size_t uCodeSize = 256;

		template <typename T>
		[[nodiscard]] static std::uintptr_t ToArgument(T value)
		{
			if constexpr (std::is_pointer_v<T>)
				return reinterpret_cast<std::uintptr_t>(value);
			else
				return static_cast<std::uintptr_t>(value);
		}

		std::uint8_t* m_pCode = nullptr;
	};

	namespace FakeInput
	{
		const HRAWINPUT hKeyboard = reinterpret_cast<HRAWINPUT>(static_cast<std::uintptr_t>(0x0BAD0001));
		const HRAWINPUT hMouse = reinterpret_cast<HRAWINPUT>(static_cast<std::uintptr_t>(0x0BAD0002));
		constexpr UINT uRawEntry = (sizeof(RAWINPUT) + 7) & ~UINT{ 7 };

		OmegaRenderHook::clInlineHook clRawData;
		OmegaRenderHook::clInlineHook clRawBuffer;
		OmegaRenderHook::clInlineHook clXInput;
		std::array<OmegaRenderHook::clInlineHook, 5> aDirectInput;
		std::array<OmegaRenderHook::clInlineHook, 5> aDirectInput7;
		std::array<OmegaRenderHook::clInlineHook, 5> aGameInput;
		bool bRawBuffer = false;
		IDirectInputDevice8W* pKeyboard = nullptr;
		IDirectInputDevice8W* pPad = nullptr;
		std::int64_t iMouseX = 0;

		void FillRaw(RAWINPUT& rInput, bool bMouse)
		{
			rInput = {};
			rInput.header.dwSize = sizeof(RAWINPUT);
			if (bMouse)
			{
				rInput.header.dwType = RIM_TYPEMOUSE;
				rInput.data.mouse.usFlags = MOUSE_MOVE_RELATIVE;
				rInput.data.mouse.usButtonFlags = RI_MOUSE_LEFT_BUTTON_DOWN;
				rInput.data.mouse.lLastX = 15;
				rInput.data.mouse.lLastY = -4;
				return;
			}
			rInput.header.dwType = RIM_TYPEKEYBOARD;
			rInput.data.keyboard.MakeCode = 0x11;
			rInput.data.keyboard.Flags = RI_KEY_MAKE;
			rInput.data.keyboard.VKey = 'W';
			rInput.data.keyboard.Message = WM_KEYDOWN;
		}

		UINT WINAPI RawData(HRAWINPUT hInput, UINT uCommand, LPVOID pData, PUINT pSize, UINT uHeaderSize)
		{
			if (hInput != hKeyboard && hInput != hMouse)
				return clRawData.Original<decltype(&RawData)>()(hInput, uCommand, pData, pSize, uHeaderSize);
			if (uCommand != RID_INPUT || !pSize)
				return static_cast<UINT>(-1);
			if (!pData)
			{
				*pSize = sizeof(RAWINPUT);
				return 0;
			}
			if (*pSize < sizeof(RAWINPUT))
				return static_cast<UINT>(-1);
			FillRaw(*static_cast<RAWINPUT*>(pData), hInput == hMouse);
			return sizeof(RAWINPUT);
		}

		UINT WINAPI RawBuffer(PRAWINPUT pData, PUINT pSize, UINT uHeaderSize)
		{
			if (!bRawBuffer)
				return clRawBuffer.Original<decltype(&RawBuffer)>()(pData, pSize, uHeaderSize);
			if (!pData)
			{
				*pSize = uRawEntry;
				return 0;
			}
			if (*pSize < 2 * uRawEntry)
				return static_cast<UINT>(-1);
			FillRaw(*pData, false);
			FillRaw(*reinterpret_cast<RAWINPUT*>(reinterpret_cast<std::uint8_t*>(pData) + uRawEntry), true);
			return 2;
		}

		DWORD WINAPI XInputState(DWORD, XINPUT_STATE* pState)
		{
			if (!pState)
				return ERROR_BAD_ARGUMENTS;
			*pState = {};
			pState->dwPacketNumber = 7;
			pState->Gamepad.wButtons = XINPUT_GAMEPAD_A;
			pState->Gamepad.sThumbLX = 20000;
			pState->Gamepad.bLeftTrigger = 200;
			return ERROR_SUCCESS;
		}

		template <bool bLegacy>
		[[nodiscard]] std::array<OmegaRenderHook::clInlineHook, 5>& DeviceHooks()
		{
			return bLegacy ? aDirectInput7 : aDirectInput;
		}

		template <bool bLegacy>
		HRESULT WINAPI Capabilities(IDirectInputDevice8W* pDevice, LPDIDEVCAPS pCaps)
		{
			const HRESULT hResult = DeviceHooks<bLegacy>()[0].Original<decltype(&Capabilities<bLegacy>)>()(pDevice, pCaps);
			if (pDevice == pPad && pCaps)
				pCaps->dwDevType = bLegacy ? 4 | (2 << 8) : DI8DEVTYPE_GAMEPAD | (DI8DEVTYPEGAMEPAD_STANDARD << 8);
			return hResult;
		}

		template <bool bLegacy>
		HRESULT WINAPI Property(IDirectInputDevice8W* pDevice, REFGUID rGuid, LPDIPROPHEADER pHeader)
		{
			if (pDevice != pPad || &rGuid != &DIPROP_RANGE || !pHeader || pHeader->dwHow != DIPH_BYOFFSET || (pHeader->dwObj != DIJOFS_X && pHeader->dwObj != DIJOFS_Y))
				return DeviceHooks<bLegacy>()[1].Original<decltype(&Property<bLegacy>)>()(pDevice, rGuid, pHeader);
			auto* pRange = reinterpret_cast<LPDIPROPRANGE>(pHeader);
			pRange->lMin = pHeader->dwObj == DIJOFS_X ? 0 : -1000;
			pRange->lMax = pHeader->dwObj == DIJOFS_X ? 65535 : 1000;
			return DI_OK;
		}

		template <bool bLegacy>
		HRESULT WINAPI DeviceState(IDirectInputDevice8W* pDevice, DWORD uSize, LPVOID pData)
		{
			if (pDevice == pPad && pData && uSize >= sizeof(DIJOYSTATE))
			{
				std::memset(pData, 0, uSize);
				auto* pState = static_cast<DIJOYSTATE*>(pData);
				pState->lX = 65535;
				pState->lY = 1000;
				pState->rgdwPOV[0] = 9000;
				pState->rgbButtons[0] = 0x80;
				return DI_OK;
			}
			if (pDevice == pKeyboard && pData && uSize >= 256)
			{
				std::memset(pData, 0, uSize);
				static_cast<BYTE*>(pData)[DIK_W] = 0x80;
				return DI_OK;
			}
			return DeviceHooks<bLegacy>()[2].Original<decltype(&DeviceState<bLegacy>)>()(pDevice, uSize, pData);
		}

		template <bool bLegacy>
		HRESULT WINAPI DeviceData(IDirectInputDevice8W* pDevice, DWORD uObjectSize, LPDIDEVICEOBJECTDATA pData, LPDWORD pInOut, DWORD uFlags)
		{
			if ((pDevice != pPad && pDevice != pKeyboard) || !pData || !pInOut || uObjectSize != sizeof(DIDEVICEOBJECTDATA))
				return DeviceHooks<bLegacy>()[3].Original<decltype(&DeviceData<bLegacy>)>()(pDevice, uObjectSize, pData, pInOut, uFlags);
			const std::array<std::pair<DWORD, DWORD>, 3> aPad = { { { static_cast<DWORD>(DIJOFS_X), 65535 }, { static_cast<DWORD>(DIJOFS_POV(0)), 9000 }, { static_cast<DWORD>(DIJOFS_BUTTON0), 0x80 } } };
			const std::size_t uEvents = pDevice == pPad ? aPad.size() : 1;
			if (*pInOut < uEvents)
				return DIERR_INVALIDPARAM;
			for (std::size_t i = 0; i < uEvents; ++i)
			{
				pData[i] = {};
				pData[i].dwOfs = pDevice == pPad ? aPad[i].first : DIK_W;
				pData[i].dwData = pDevice == pPad ? aPad[i].second : 0x80;
			}
			*pInOut = static_cast<DWORD>(uEvents);
			return DI_OK;
		}

		template <bool bLegacy>
		HRESULT WINAPI ObjectInfo(IDirectInputDevice8W* pDevice, LPDIDEVICEOBJECTINSTANCEW pInstance, DWORD uObject, DWORD uHow)
		{
			if (pDevice != pPad || uHow != DIPH_BYOFFSET)
				return DeviceHooks<bLegacy>()[4].Original<decltype(&ObjectInfo<bLegacy>)>()(pDevice, pInstance, uObject, uHow);
			if (!pInstance || pInstance->dwSize != sizeof(DIDEVICEOBJECTINSTANCEW))
				return DIERR_INVALIDPARAM;
			if (uObject == DIJOFS_X || uObject == DIJOFS_Y)
				pInstance->dwType = DIDFT_ABSAXIS | DIDFT_MAKEINSTANCE(uObject / 4);
			else if (uObject == DIJOFS_POV(0))
				pInstance->dwType = DIDFT_POV | DIDFT_MAKEINSTANCE(0);
			else
				return DIERR_OBJECTNOTFOUND;
			pInstance->dwOfs = uObject;
			return DI_OK;
		}

		uint32_t STDMETHODCALLTYPE GameInputAxes(IGameInputReading*, uint32_t uCount, float* pAxes)
		{
			const uint32_t uWritten = std::min<uint32_t>(uCount, 2);
			std::fill_n(pAxes, uWritten, 0.9f);
			return uWritten;
		}

		uint32_t STDMETHODCALLTYPE GameInputSwitches(IGameInputReading*, uint32_t uCount, GameInputSwitchPosition* pSwitches)
		{
			const uint32_t uWritten = std::min<uint32_t>(uCount, 1);
			std::fill_n(pSwitches, uWritten, GameInputSwitchUp);
			return uWritten;
		}

		uint32_t STDMETHODCALLTYPE GameInputKeys(IGameInputReading*, uint32_t uCount, GameInputKeyState* pKeys)
		{
			if (!uCount || !pKeys)
				return 0;
			pKeys[0] = { 0x11, 'w', 'W', false };
			return 1;
		}

		bool STDMETHODCALLTYPE GameInputMouse(IGameInputReading*, GameInputMouseState* pMouse)
		{
			*pMouse = {};
			pMouse->buttons = GameInputMouseLeftButton;
			pMouse->positionX = iMouseX;
			pMouse->positionY = 50;
			return true;
		}

		bool STDMETHODCALLTYPE GameInputGamepad(IGameInputReading*, GameInputGamepadState* pGamepad)
		{
			*pGamepad = {};
			pGamepad->buttons = GameInputGamepadA;
			pGamepad->leftThumbstickX = 0.8f;
			pGamepad->leftTrigger = 1.f;
			return true;
		}
	}

	struct clSwallowListener final : OmegaRenderHook::clRenderListener
	{
		bool bSwallow = false;

		bool OnWindowMessage(HWND, UINT uMessage, WPARAM, LPARAM, LRESULT& rResult) override
		{
			const bool bInput = (uMessage >= WM_KEYFIRST && uMessage <= WM_KEYLAST) || (uMessage >= WM_MOUSEFIRST && uMessage <= WM_MOUSELAST);
			rResult = 0;
			return bSwallow && bInput;
		}
	};

	std::vector<std::pair<UINT, WPARAM>> g_aGameMessages;

	LRESULT CALLBACK GameWindowProc(HWND hWindow, UINT uMessage, WPARAM uWParam, LPARAM iLParam)
	{
		if ((uMessage >= WM_KEYFIRST && uMessage <= WM_KEYLAST) || (uMessage >= WM_MOUSEFIRST && uMessage <= WM_MOUSELAST))
			g_aGameMessages.emplace_back(uMessage, uWParam);
		return DefWindowProcW(hWindow, uMessage, uWParam, iLParam);
	}

	[[nodiscard]] std::size_t CountGameMessages(UINT uMessage, WPARAM uWParam)
	{
		return static_cast<std::size_t>(std::ranges::count(g_aGameMessages, std::pair<UINT, WPARAM>{ uMessage, uWParam }));
	}

	void TestInputBlock()
	{
		std::puts("input block");
		clForeignCall clForeign;
		void* pGetKeyState = reinterpret_cast<void*>(GetProcAddress(GetModuleHandleW(L"user32.dll"), "GetKeyState"));
		void* pGetKeyboardState = reinterpret_cast<void*>(GetProcAddress(GetModuleHandleW(L"user32.dll"), "GetKeyboardState"));
		if (!clForeign.IsValid() || !pGetKeyState || !pGetKeyboardState)
			return Check("foreign call stub available", false);

		std::array<BYTE, 256> aSaved{};
		GetKeyboardState(aSaved.data());
		std::array<BYTE, 256> aPressed = aSaved;
		aPressed['K'] = 0x80;
		aPressed[VK_XBUTTON2] = 0x80;
		SetKeyboardState(aPressed.data());

		const auto ForeignKeyDown = [&](int iKey) { return (static_cast<SHORT>(clForeign.Call(pGetKeyState, static_cast<std::uintptr_t>(iKey))) & 0x8000) != 0; };
		Check("foreign GetKeyState sees a pressed key before install", ForeignKeyDown('K'));
		Check("input block installs", OmegaRenderHook::InputBlock::Install());

		OmegaRenderHook::InputBlock::SetBlocking({ true, false, false });
		Check("blocked keyboard hides the key from foreign callers", !ForeignKeyDown('K'));
		Check("own module still sees the key while blocked", (GetKeyState('K') & 0x8000) != 0);
		Check("mouse buttons stay visible while only the keyboard is blocked", ForeignKeyDown(VK_XBUTTON2));

		std::array<BYTE, 256> aForeign{};
		static_cast<void>(clForeign.Call(pGetKeyboardState, reinterpret_cast<std::uintptr_t>(aForeign.data())));
		Check("blocked keyboard state is cleared for foreign callers", (aForeign['K'] & 0x80) == 0 && (aForeign[VK_XBUTTON2] & 0x80) != 0);

		void* pSetCursor = reinterpret_cast<void*>(GetProcAddress(GetModuleHandleW(L"user32.dll"), "SetCursor"));
		const HCURSOR hArrow = LoadCursorW(nullptr, IDC_ARROW);
		const HCURSOR hBeam = LoadCursorW(nullptr, IDC_IBEAM);
		SetCursor(hArrow);
		OmegaRenderHook::InputBlock::SetBlocking({ true, true, false });
		static_cast<void>(clForeign.Call(pSetCursor, hBeam));
		Check("blocked mouse keeps the game from changing the cursor", GetCursor() == hArrow);
		SetCursor(hBeam);
		Check("own module still sets the cursor while blocked", GetCursor() == hBeam);
		OmegaRenderHook::InputBlock::SetBlocking({ true, false, false });
		static_cast<void>(clForeign.Call(pSetCursor, hArrow));
		Check("unblocked mouse lets the game set the cursor again", GetCursor() == hArrow);

		OmegaRenderHook::InputBlock::SetBlocking({});
		Check("unblocking restores the key", ForeignKeyDown('K'));
		Check("input block uninstalls", OmegaRenderHook::InputBlock::Uninstall() && !OmegaRenderHook::InputBlock::IsInstalled());
		OmegaRenderHook::InputBlock::SetBlocking({ true, false, false });
		Check("blocking without hooks has no effect", ForeignKeyDown('K'));
		OmegaRenderHook::InputBlock::SetBlocking({});
		SetKeyboardState(aSaved.data());
	}

	void TestInputDevices()
	{
		std::puts("input block devices");
		clForeignCall clForeign;
		const HMODULE hUser32 = GetModuleHandleW(L"user32.dll");
		void* pRawData = reinterpret_cast<void*>(GetProcAddress(hUser32, "GetRawInputData"));
		void* pRawBuffer = reinterpret_cast<void*>(GetProcAddress(hUser32, "GetRawInputBuffer"));
		const HMODULE hXInput = LoadLibraryW(L"xinput1_4.dll");
		void* pXInput = hXInput ? reinterpret_cast<void*>(GetProcAddress(hXInput, "XInputGetState")) : nullptr;
		const HMODULE hDirectInput = LoadLibraryW(L"dinput8.dll");
		using FnDirectInput8Create = HRESULT(WINAPI*)(HINSTANCE, DWORD, REFIID, LPVOID*, LPUNKNOWN);
		const auto pfnDirectInput8Create = hDirectInput ? reinterpret_cast<FnDirectInput8Create>(GetProcAddress(hDirectInput, "DirectInput8Create")) : nullptr;
		if (!clForeign.IsValid() || !pRawData || !pRawBuffer || !pXInput || !pfnDirectInput8Create)
			return Check("raw input, xinput and directinput are available", false);

		IDirectInput8W* pDirectInput = nullptr;
		const bool bDevices = SUCCEEDED(pfnDirectInput8Create(GetModuleHandleW(nullptr), DIRECTINPUT_VERSION, IID_IDirectInput8W, reinterpret_cast<void**>(&pDirectInput), nullptr)) &&
			SUCCEEDED(pDirectInput->CreateDevice(GUID_SysKeyboard, &FakeInput::pKeyboard, nullptr)) && SUCCEEDED(pDirectInput->CreateDevice(GUID_SysMouse, &FakeInput::pPad, nullptr));
		Check("directinput test devices created", bDevices);
		if (!bDevices)
			return;
		void** ppDeviceTable = *reinterpret_cast<void***>(FakeInput::pKeyboard);
		Check("directinput keyboard and joystick devices share one vtable", ppDeviceTable == *reinterpret_cast<void***>(FakeInput::pPad));

		bool bFakes = FakeInput::clRawData.Install(pRawData, reinterpret_cast<void*>(&FakeInput::RawData)) && FakeInput::clRawBuffer.Install(pRawBuffer, reinterpret_cast<void*>(&FakeInput::RawBuffer)) &&
			FakeInput::clXInput.Install(pXInput, reinterpret_cast<void*>(&FakeInput::XInputState));
		const std::array<std::pair<std::size_t, void*>, 5> aDirectInputFakes = { {
			{ 3, reinterpret_cast<void*>(&FakeInput::Capabilities<false>) },
			{ 5, reinterpret_cast<void*>(&FakeInput::Property<false>) },
			{ 9, reinterpret_cast<void*>(&FakeInput::DeviceState<false>) },
			{ 10, reinterpret_cast<void*>(&FakeInput::DeviceData<false>) },
			{ 14, reinterpret_cast<void*>(&FakeInput::ObjectInfo<false>) },
		} };
		for (std::size_t i = 0; i < aDirectInputFakes.size(); ++i)
			bFakes = FakeInput::aDirectInput[i].Install(ppDeviceTable[aDirectInputFakes[i].first], aDirectInputFakes[i].second) && bFakes;
		Check("fake device drivers installed", bFakes);

		Check("input block installs over the fake drivers", OmegaRenderHook::InputBlock::Install());

		RAWINPUT stRaw{};
		UINT uSize = sizeof(stRaw);
		const auto ForeignRaw = [&](HRAWINPUT hInput) { uSize = sizeof(stRaw); return static_cast<UINT>(clForeign.Call(pRawData, hInput, RID_INPUT, &stRaw, &uSize, sizeof(RAWINPUTHEADER))); };
		Check("raw keyboard reaches foreign callers while unblocked", ForeignRaw(FakeInput::hKeyboard) == sizeof(RAWINPUT) && stRaw.data.keyboard.Message == WM_KEYDOWN && !(stRaw.data.keyboard.Flags & RI_KEY_BREAK));
		OmegaRenderHook::InputBlock::SetBlocking({ true, true, false });
		Check("blocked raw keyboard is turned into a key release", ForeignRaw(FakeInput::hKeyboard) == sizeof(RAWINPUT) && stRaw.data.keyboard.Message == WM_KEYUP && (stRaw.data.keyboard.Flags & RI_KEY_BREAK) && stRaw.data.keyboard.VKey == 'W');
		Check("blocked raw mouse has no motion and releases its buttons", ForeignRaw(FakeInput::hMouse) == sizeof(RAWINPUT) && stRaw.data.mouse.lLastX == 0 && stRaw.data.mouse.lLastY == 0 && stRaw.data.mouse.usButtonFlags == RI_MOUSE_LEFT_BUTTON_UP);
		uSize = sizeof(stRaw);
		Check("own module still reads raw input while blocked", GetRawInputData(FakeInput::hMouse, RID_INPUT, &stRaw, &uSize, sizeof(RAWINPUTHEADER)) == sizeof(RAWINPUT) && stRaw.data.mouse.lLastX == 15);

		alignas(8) std::array<std::uint8_t, FakeInput::uRawEntry * 2> aBuffer{};
		UINT uBufferSize = static_cast<UINT>(aBuffer.size());
		FakeInput::bRawBuffer = true;
		const UINT uEntries = static_cast<UINT>(clForeign.Call(pRawBuffer, aBuffer.data(), &uBufferSize, sizeof(RAWINPUTHEADER)));
		FakeInput::bRawBuffer = false;
		const auto& rBufferedKey = *reinterpret_cast<const RAWINPUT*>(aBuffer.data());
		const auto& rBufferedMouse = *reinterpret_cast<const RAWINPUT*>(aBuffer.data() + FakeInput::uRawEntry);
		Check("blocked raw input buffer is neutralized entry by entry", uEntries == 2 && (rBufferedKey.data.keyboard.Flags & RI_KEY_BREAK) && rBufferedMouse.data.mouse.lLastX == 0 && rBufferedMouse.data.mouse.usButtonFlags == RI_MOUSE_LEFT_BUTTON_UP);
		OmegaRenderHook::InputBlock::SetBlocking({});

		XINPUT_STATE stPad{};
		const auto ForeignXInput = [&] { stPad = {}; return static_cast<DWORD>(clForeign.Call(pXInput, 0, &stPad)); };
		Check("xinput reaches foreign callers while unblocked", ForeignXInput() == ERROR_SUCCESS && stPad.Gamepad.wButtons == XINPUT_GAMEPAD_A && stPad.Gamepad.sThumbLX == 20000);
		OmegaRenderHook::InputBlock::SetBlocking({ false, false, true });
		Check("blocked xinput reports a neutral gamepad", ForeignXInput() == ERROR_SUCCESS && stPad.Gamepad.wButtons == 0 && stPad.Gamepad.sThumbLX == 0 && stPad.Gamepad.bLeftTrigger == 0 && stPad.dwPacketNumber == 7);
		stPad = {};
		Check("own module still reads xinput while blocked", reinterpret_cast<DWORD(WINAPI*)(DWORD, XINPUT_STATE*)>(pXInput)(0, &stPad) == ERROR_SUCCESS && stPad.Gamepad.wButtons == XINPUT_GAMEPAD_A);
		OmegaRenderHook::InputBlock::SetBlocking({});

		void* pDeviceState = ppDeviceTable[9];
		void* pDeviceData = ppDeviceTable[10];
		DIJOYSTATE stJoy{};
		const auto ForeignJoy = [&] { stJoy = {}; return static_cast<HRESULT>(clForeign.Call(pDeviceState, FakeInput::pPad, sizeof(stJoy), &stJoy)); };
		std::array<BYTE, 256> aKeys{};
		const auto ForeignKeys = [&] { aKeys = {}; return static_cast<HRESULT>(clForeign.Call(pDeviceState, FakeInput::pKeyboard, aKeys.size(), aKeys.data())); };
		Check("directinput state reaches foreign callers while unblocked", SUCCEEDED(ForeignJoy()) && stJoy.lX == 65535 && stJoy.rgbButtons[0] == 0x80 && SUCCEEDED(ForeignKeys()) && aKeys[DIK_W] == 0x80);
		OmegaRenderHook::InputBlock::SetBlocking({ true, false, false });
		Check("blocked directinput keyboard state is released", SUCCEEDED(ForeignKeys()) && aKeys[DIK_W] == 0);
		Check("directinput joystick is untouched while only the keyboard is blocked", SUCCEEDED(ForeignJoy()) && stJoy.lX == 65535);
		OmegaRenderHook::InputBlock::SetBlocking({ false, false, true });
		Check("blocked directinput joystick has centered axes", SUCCEEDED(ForeignJoy()) && stJoy.lX == 32767 && stJoy.lY == 0, std::format("x {} y {}", stJoy.lX, stJoy.lY));
		Check("blocked directinput joystick has a centered pov and released buttons", stJoy.rgdwPOV[0] == 0xFFFFFFFF && stJoy.rgbButtons[0] == 0);

		std::array<DIDEVICEOBJECTDATA, 3> aEvents{};
		DWORD uEvents = static_cast<DWORD>(aEvents.size());
		const HRESULT hPadEvents = static_cast<HRESULT>(clForeign.Call(pDeviceData, FakeInput::pPad, sizeof(DIDEVICEOBJECTDATA), aEvents.data(), &uEvents, 0));
		Check("blocked directinput joystick events are neutral", SUCCEEDED(hPadEvents) && uEvents == 3 && aEvents[0].dwData == 32767 && aEvents[1].dwData == 0xFFFFFFFF && aEvents[2].dwData == 0);
		OmegaRenderHook::InputBlock::SetBlocking({ true, false, false });
		uEvents = 1;
		const HRESULT hKeyEvents = static_cast<HRESULT>(clForeign.Call(pDeviceData, FakeInput::pKeyboard, sizeof(DIDEVICEOBJECTDATA), aEvents.data(), &uEvents, 0));
		Check("blocked directinput keyboard events are releases", SUCCEEDED(hKeyEvents) && uEvents == 1 && aEvents[0].dwOfs == DIK_W && aEvents[0].dwData == 0);
		OmegaRenderHook::InputBlock::SetBlocking({});

		Check("input block uninstalls over the fake drivers", OmegaRenderHook::InputBlock::Uninstall());
		for (auto& rHook : FakeInput::aDirectInput)
			rHook.Uninstall();
		FakeInput::clXInput.Uninstall();
		FakeInput::clRawBuffer.Uninstall();
		FakeInput::clRawData.Uninstall();
		Check("foreign xinput is unblocked once everything is removed", ForeignXInput() != ERROR_SUCCESS || stPad.Gamepad.wButtons != XINPUT_GAMEPAD_A || stPad.dwPacketNumber != 7);
		FakeInput::pPad->Release();
		FakeInput::pKeyboard->Release();
		pDirectInput->Release();
		FakeInput::pPad = nullptr;
		FakeInput::pKeyboard = nullptr;
	}

	void TestDirectInput7()
	{
		std::puts("input block directinput7");
		clForeignCall clForeign;
		const HMODULE hDirectInput = LoadLibraryW(L"dinput.dll");
		using FnDirectInputCreateEx = HRESULT(WINAPI*)(HINSTANCE, DWORD, REFIID, LPVOID*, LPUNKNOWN);
		const auto pfnCreate = hDirectInput ? reinterpret_cast<FnDirectInputCreateEx>(GetProcAddress(hDirectInput, "DirectInputCreateEx")) : nullptr;
		IDirectInput7W* pDirectInput = nullptr;
		IDirectInputDeviceW* pKeyboard = nullptr;
		IDirectInputDeviceW* pPad = nullptr;
		const bool bDevices = clForeign.IsValid() && pfnCreate && SUCCEEDED(pfnCreate(GetModuleHandleW(nullptr), 0x0700, IID_IDirectInput7W, reinterpret_cast<void**>(&pDirectInput), nullptr)) &&
			SUCCEEDED(pDirectInput->CreateDevice(GUID_SysKeyboard, &pKeyboard, nullptr)) && SUCCEEDED(pDirectInput->CreateDevice(GUID_SysMouse, &pPad, nullptr));
		Check("directinput7 test devices created", bDevices);
		if (!bDevices)
			return;
		FakeInput::pKeyboard = reinterpret_cast<IDirectInputDevice8W*>(pKeyboard);
		FakeInput::pPad = reinterpret_cast<IDirectInputDevice8W*>(pPad);
		void** ppDeviceTable = *reinterpret_cast<void***>(pKeyboard);
		const std::array<std::pair<std::size_t, void*>, 5> aFakes = { {
			{ 3, reinterpret_cast<void*>(&FakeInput::Capabilities<true>) },
			{ 5, reinterpret_cast<void*>(&FakeInput::Property<true>) },
			{ 9, reinterpret_cast<void*>(&FakeInput::DeviceState<true>) },
			{ 10, reinterpret_cast<void*>(&FakeInput::DeviceData<true>) },
			{ 14, reinterpret_cast<void*>(&FakeInput::ObjectInfo<true>) },
		} };
		bool bFakes = ppDeviceTable == *reinterpret_cast<void***>(pPad);
		for (std::size_t i = 0; i < aFakes.size(); ++i)
			bFakes = FakeInput::aDirectInput7[i].Install(ppDeviceTable[aFakes[i].first], aFakes[i].second) && bFakes;
		Check("fake directinput7 drivers installed", bFakes);
		Check("input block installs over directinput7", OmegaRenderHook::InputBlock::Install());

		void* pDeviceState = ppDeviceTable[9];
		void* pDeviceData = ppDeviceTable[10];
		DIJOYSTATE stJoy{};
		const auto ForeignJoy = [&] { stJoy = {}; return static_cast<HRESULT>(clForeign.Call(pDeviceState, pPad, sizeof(stJoy), &stJoy)); };
		std::array<BYTE, 256> aKeys{};
		const auto ForeignKeys = [&] { aKeys = {}; return static_cast<HRESULT>(clForeign.Call(pDeviceState, pKeyboard, aKeys.size(), aKeys.data())); };
		Check("directinput7 state reaches foreign callers while unblocked", SUCCEEDED(ForeignJoy()) && stJoy.lX == 65535 && SUCCEEDED(ForeignKeys()) && aKeys[DIK_W] == 0x80);
		OmegaRenderHook::InputBlock::SetBlocking({ true, false, false });
		Check("blocked directinput7 keyboard state is released", SUCCEEDED(ForeignKeys()) && aKeys[DIK_W] == 0);
		Check("directinput7 joystick is untouched while only the keyboard is blocked", SUCCEEDED(ForeignJoy()) && stJoy.lX == 65535);
		OmegaRenderHook::InputBlock::SetBlocking({ false, false, true });
		Check("blocked directinput7 joystick is centered", SUCCEEDED(ForeignJoy()) && stJoy.lX == 32767 && stJoy.lY == 0 && stJoy.rgdwPOV[0] == 0xFFFFFFFF && stJoy.rgbButtons[0] == 0, std::format("x {} y {}", stJoy.lX, stJoy.lY));
		std::array<DIDEVICEOBJECTDATA, 3> aEvents{};
		DWORD uEvents = static_cast<DWORD>(aEvents.size());
		const HRESULT hPadEvents = static_cast<HRESULT>(clForeign.Call(pDeviceData, pPad, sizeof(DIDEVICEOBJECTDATA), aEvents.data(), &uEvents, 0));
		Check("blocked directinput7 joystick events are neutral", SUCCEEDED(hPadEvents) && uEvents == 3 && aEvents[0].dwData == 32767 && aEvents[1].dwData == 0xFFFFFFFF && aEvents[2].dwData == 0);
		stJoy = {};
		Check("own module still reads directinput7 while blocked", SUCCEEDED(pPad->GetDeviceState(sizeof(stJoy), &stJoy)) && stJoy.lX == 65535);
		OmegaRenderHook::InputBlock::SetBlocking({});

		Check("input block uninstalls over directinput7", OmegaRenderHook::InputBlock::Uninstall());
		for (auto& rHook : FakeInput::aDirectInput7)
			rHook.Uninstall();
		pPad->Release();
		pKeyboard->Release();
		pDirectInput->Release();
		FakeInput::pPad = nullptr;
		FakeInput::pKeyboard = nullptr;
	}

	void TestGameInput()
	{
		std::puts("input block gameinput");
		clForeignCall clForeign;
		const HMODULE hGameInput = LoadLibraryW(L"GameInput.dll");
		using FnGameInputCreate = HRESULT(WINAPI*)(IGameInput**);
		const auto pfnCreate = hGameInput ? reinterpret_cast<FnGameInputCreate>(GetProcAddress(hGameInput, "GameInputCreate")) : nullptr;
		IGameInput* pInput = nullptr;
		IGameInputReading* pReading = nullptr;
		HRESULT hReading = E_FAIL;
		if (clForeign.IsValid() && pfnCreate && SUCCEEDED(hReading = pfnCreate(&pInput)) && pInput)
		{
			hReading = pInput->GetCurrentReading(GameInputKindKeyboard | GameInputKindMouse, nullptr, &pReading);
			IGameInputDevice* pDevice = nullptr;
			GameInputCallbackToken uToken = 0;
			if (!pReading && SUCCEEDED(pInput->RegisterDeviceCallback(nullptr, GameInputKindKeyboard | GameInputKindMouse, GameInputDeviceConnected, GameInputBlockingEnumeration, &pDevice,
				[](GameInputCallbackToken, void* pContext, IGameInputDevice* pFound, uint64_t, GameInputDeviceStatus, GameInputDeviceStatus)
				{
					auto** ppDevice = static_cast<IGameInputDevice**>(pContext);
					if (!*ppDevice)
					{
						pFound->AddRef();
						*ppDevice = pFound;
					}
				}, &uToken)))
				pInput->UnregisterCallback(uToken, 1'000'000);
			if (!pReading && pDevice)
			{
				hReading = pInput->GetCurrentReading(GameInputKindKeyboard | GameInputKindMouse, pDevice, &pReading);
				if (!pReading)
					hReading = pInput->GetTemporalReading(pInput->GetCurrentTimestamp(), pDevice, &pReading);
			}
			if (pDevice)
				pDevice->Release();
		}
		if (!pReading)
		{
			if (pInput)
				pInput->Release();
			std::printf("  [skip] gameinput runtime or keyboard and mouse readings are not available (0x%08X)\n", static_cast<unsigned>(hReading));
			return;
		}

		void** ppReadingTable = *reinterpret_cast<void***>(pReading);
		const std::array<std::pair<std::size_t, void*>, 5> aFakes = { {
			{ 9, reinterpret_cast<void*>(&FakeInput::GameInputAxes) },
			{ 13, reinterpret_cast<void*>(&FakeInput::GameInputSwitches) },
			{ 15, reinterpret_cast<void*>(&FakeInput::GameInputKeys) },
			{ 16, reinterpret_cast<void*>(&FakeInput::GameInputMouse) },
			{ 22, reinterpret_cast<void*>(&FakeInput::GameInputGamepad) },
		} };
		bool bFakes = true;
		for (std::size_t i = 0; i < aFakes.size(); ++i)
			bFakes = FakeInput::aGameInput[i].Install(ppReadingTable[aFakes[i].first], aFakes[i].second) && bFakes;
		Check("fake gameinput readings installed", bFakes);
		Check("input block installs over gameinput", OmegaRenderHook::InputBlock::Install());

		std::array<float, 2> aAxes{};
		std::array<GameInputSwitchPosition, 1> aSwitches{};
		std::array<GameInputKeyState, 4> aKeys{};
		GameInputMouseState stMouse{};
		GameInputGamepadState stGamepad{};
		const auto ForeignKeys = [&] { return static_cast<uint32_t>(clForeign.Call(ppReadingTable[15], pReading, aKeys.size(), aKeys.data())); };
		const auto ForeignMouse = [&] { stMouse = {}; return (clForeign.Call(ppReadingTable[16], pReading, &stMouse) & 0xFF) != 0; };
		const auto ForeignGamepad = [&] { stGamepad = {}; return (clForeign.Call(ppReadingTable[22], pReading, &stGamepad) & 0xFF) != 0; };

		FakeInput::iMouseX = 100;
		Check("gameinput reaches foreign callers while unblocked", ForeignKeys() == 1 && ForeignMouse() && stMouse.buttons == GameInputMouseLeftButton && stMouse.positionX == 100 && ForeignGamepad() && stGamepad.buttons == GameInputGamepadA);
		OmegaRenderHook::InputBlock::SetBlocking({ true, true, true });
		FakeInput::iMouseX = 300;
		Check("blocked gameinput keyboard reports no keys", ForeignKeys() == 0);
		Check("blocked gameinput mouse is frozen with no buttons", ForeignMouse() && stMouse.buttons == GameInputMouseNone && stMouse.positionX == 100, std::format("x {}", stMouse.positionX));
		Check("blocked gameinput gamepad is neutral", ForeignGamepad() && stGamepad.buttons == GameInputGamepadNone && stGamepad.leftThumbstickX == 0.f && stGamepad.leftTrigger == 0.f);
		const auto uAxes = static_cast<uint32_t>(clForeign.Call(ppReadingTable[9], pReading, aAxes.size(), aAxes.data()));
		const auto uSwitches = static_cast<uint32_t>(clForeign.Call(ppReadingTable[13], pReading, aSwitches.size(), aSwitches.data()));
		Check("blocked gameinput controller axes are centered and switches neutral", uAxes == 2 && aAxes[0] == 0.5f && aAxes[1] == 0.5f && uSwitches == 1 && aSwitches[0] == GameInputSwitchCenter);
		Check("own module still reads gameinput while blocked", pReading->GetGamepadState(&stGamepad) && stGamepad.buttons == GameInputGamepadA);
		OmegaRenderHook::InputBlock::SetBlocking({});
		Check("unblocking restores gameinput", ForeignMouse() && stMouse.positionX == 300 && stMouse.buttons == GameInputMouseLeftButton);

		Check("input block uninstalls over gameinput", OmegaRenderHook::InputBlock::Uninstall());
		for (auto& rHook : FakeInput::aGameInput)
			rHook.Uninstall();
		pReading->Release();
		pInput->Release();
	}

	void TestHeldInputRelease()
	{
		std::puts("input block held input");
		WNDCLASSEXW stClass{ sizeof(WNDCLASSEXW) };
		stClass.lpfnWndProc = &GameWindowProc;
		stClass.hInstance = GetModuleHandleW(nullptr);
		stClass.lpszClassName = L"OmegaWareTestsHeldInput";
		RegisterClassExW(&stClass);
		const HWND hWindow = CreateWindowExW(0, stClass.lpszClassName, L"held input", WS_POPUP, 0, 0, 64, 64, nullptr, nullptr, stClass.hInstance, nullptr);
		clSwallowListener clListener;
		if (!hWindow || !OmegaRenderHook::WindowHook::Attach(hWindow, &clListener))
		{
			if (hWindow)
				DestroyWindow(hWindow);
			return Check("test window subclassed", false);
		}
		const auto Pump = [&] { MSG stMessage{}; while (PeekMessageW(&stMessage, hWindow, 0, 0, PM_REMOVE)) DispatchMessageW(&stMessage); };
		const LPARAM iKeyDown = 1 | (0x11 << 16);
		const LPARAM iKeyUp = iKeyDown | static_cast<LPARAM>(0xC0000000u);

		g_aGameMessages.clear();
		SendMessageW(hWindow, WM_KEYDOWN, 'W', iKeyDown);
		SendMessageW(hWindow, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(10, 10));
		clListener.bSwallow = true;
		OmegaRenderHook::InputBlock::SetBlocking({ true, true, false });
		Pump();
		Check("keys the game saw pressed are released when blocking starts", CountGameMessages(WM_KEYUP, 'W') == 1);
		Check("mouse buttons the game saw pressed are released when blocking starts", CountGameMessages(WM_LBUTTONUP, 0) == 1);
		SendMessageW(hWindow, WM_KEYUP, 'W', iKeyUp);
		Check("the real release after a synthesized one is not sent twice", CountGameMessages(WM_KEYUP, 'W') == 1);
		SendMessageW(hWindow, WM_KEYDOWN, 'E', iKeyDown);
		Check("new presses stay hidden while the menu swallows input", CountGameMessages(WM_KEYDOWN, 'E') == 0);
		OmegaRenderHook::InputBlock::SetBlocking({});
		Pump();

		clListener.bSwallow = false;
		SendMessageW(hWindow, WM_KEYDOWN, 'A', iKeyDown);
		clListener.bSwallow = true;
		SendMessageW(hWindow, WM_KEYUP, 'A', iKeyUp);
		Check("a swallowed release of a key the game saw pressed still reaches the game", CountGameMessages(WM_KEYUP, 'A') == 1);
		SendMessageW(hWindow, WM_KEYUP, 'E', iKeyUp);
		Check("a swallowed release of a key the game never saw stays hidden", CountGameMessages(WM_KEYUP, 'E') == 0);

		Check("test window unsubclassed", OmegaRenderHook::WindowHook::Detach());
		DestroyWindow(hWindow);
		UnregisterClassW(stClass.lpszClassName, stClass.hInstance);
	}

	void TestProxy()
	{
		std::printf("proxy (%s)\n", OMEGAWARE_PROXY_DLL);
		wchar_t szExe[MAX_PATH]{};
		GetModuleFileNameW(nullptr, szExe, MAX_PATH);
		const std::filesystem::path pathProxy = std::filesystem::path(szExe).parent_path().parent_path() / "OmegaWareProxy" / OMEGAWARE_PROXY_DLL;
		if (!std::filesystem::exists(pathProxy))
			return Check("proxy dll is built next to the tests", false, pathProxy.string());

		wchar_t szSystem[MAX_PATH]{};
		GetSystemDirectoryW(szSystem, MAX_PATH);
		const HMODULE hReal = LoadLibraryW((std::filesystem::path(szSystem) / OMEGAWARE_PROXY_DLL).c_str());
		SetEnvironmentVariableW(L"OMEGAWARE_DISABLE", L"1");
		const HMODULE hProxy = LoadLibraryW(pathProxy.c_str());
		SetEnvironmentVariableW(L"OMEGAWARE_DISABLE", nullptr);
		Check("proxy loads with the framework disabled", hProxy && hReal && hProxy != hReal);
		if (!hProxy || !hReal || hProxy == hReal)
			return;

		const std::vector<stExport> aReal = ReadExports(hReal);
		const std::vector<stExport> aProxy = ReadExports(hProxy);
		const bool bSameTable = std::ranges::equal(aReal, aProxy, [](const stExport& rLeft, const stExport& rRight) { return rLeft.uOrdinal == rRight.uOrdinal && rLeft.strName == rRight.strName; });
		Check("proxy exports the same names and ordinals as the system dll", !aReal.empty() && bSameTable, std::format("{} vs {} exports", aReal.size(), aProxy.size()));

		std::string strMismatches;
		for (const stExport& rExport : aReal)
		{
			const LPCSTR szLookup = rExport.strName.empty() ? MAKEINTRESOURCEA(rExport.uOrdinal) : rExport.strName.c_str();
			if (GetProcAddress(hProxy, szLookup) != GetProcAddress(hReal, szLookup))
				strMismatches += rExport.strName.empty() ? std::format(" #{}", rExport.uOrdinal) : " " + rExport.strName;
		}
		Check("every proxy export resolves to the system dll's function", strMismatches.empty(), strMismatches);

		if (std::string_view(OMEGAWARE_PROXY_DLL) == "version.dll")
			TestVersionProxy(hProxy);
		FreeLibrary(hReal);
	}

	void TestVersionProxy(HMODULE hProxy)
	{
		using FnSize = DWORD(WINAPI*)(LPCWSTR, LPDWORD);
		using FnInfo = BOOL(WINAPI*)(LPCWSTR, DWORD, DWORD, LPVOID);
		using FnQuery = BOOL(WINAPI*)(LPCVOID, LPCWSTR, LPVOID*, PUINT);
		using FnLanguage = DWORD(WINAPI*)(DWORD, LPWSTR, DWORD);
		using FnSizeEx = DWORD(WINAPI*)(DWORD, LPCWSTR, LPDWORD);
		const auto pfnSize = reinterpret_cast<FnSize>(GetProcAddress(hProxy, "GetFileVersionInfoSizeW"));
		const auto pfnInfo = reinterpret_cast<FnInfo>(GetProcAddress(hProxy, "GetFileVersionInfoW"));
		const auto pfnQuery = reinterpret_cast<FnQuery>(GetProcAddress(hProxy, "VerQueryValueW"));
		const auto pfnLanguage = reinterpret_cast<FnLanguage>(GetProcAddress(hProxy, "VerLanguageNameW"));
		const auto pfnSizeEx = reinterpret_cast<FnSizeEx>(GetProcAddress(hProxy, "GetFileVersionInfoSizeExW"));
		Check("proxy exports resolve", pfnSize && pfnInfo && pfnQuery && pfnLanguage && pfnSizeEx);
		if (!pfnSize || !pfnInfo || !pfnQuery || !pfnLanguage || !pfnSizeEx)
			return;

		wchar_t szSystem[MAX_PATH]{};
		GetSystemDirectoryW(szSystem, MAX_PATH);
		const std::wstring strFile = std::wstring(szSystem) + L"\\kernel32.dll";

		DWORD uRealHandle = 0;
		DWORD uProxyHandle = 0;
		const DWORD uRealSize = GetFileVersionInfoSizeW(strFile.c_str(), &uRealHandle);
		const DWORD uProxySize = pfnSize(strFile.c_str(), &uProxyHandle);
		Check("GetFileVersionInfoSizeW matches the real dll", uRealSize && uRealSize == uProxySize, std::format("{} vs {}", uRealSize, uProxySize));
		Check("GetFileVersionInfoSizeExW matches the real dll", pfnSizeEx(0, strFile.c_str(), &uProxyHandle) == GetFileVersionInfoSizeExW(0, strFile.c_str(), &uRealHandle));

		std::vector<unsigned char> aReal(uRealSize), aProxy(uRealSize);
		const BOOL bReal = GetFileVersionInfoW(strFile.c_str(), 0, uRealSize, aReal.data());
		const BOOL bProxy = pfnInfo(strFile.c_str(), 0, uRealSize, aProxy.data());
		Check("GetFileVersionInfoW returns the same block", bReal && bProxy && aReal == aProxy);

		VS_FIXEDFILEINFO* pRealFixed = nullptr;
		VS_FIXEDFILEINFO* pProxyFixed = nullptr;
		UINT uRealLength = 0;
		UINT uProxyLength = 0;
		const BOOL bRealQuery = VerQueryValueW(aReal.data(), L"\\", reinterpret_cast<LPVOID*>(&pRealFixed), &uRealLength);
		const BOOL bProxyQuery = pfnQuery(aProxy.data(), L"\\", reinterpret_cast<LPVOID*>(&pProxyFixed), &uProxyLength);
		Check("VerQueryValueW returns the same fixed info", bRealQuery && bProxyQuery && uRealLength == uProxyLength && pProxyFixed && std::memcmp(pRealFixed, pProxyFixed, sizeof(VS_FIXEDFILEINFO)) == 0);

		wchar_t szReal[128]{};
		wchar_t szProxy[128]{};
		const DWORD uRealLanguage = VerLanguageNameW(0x409, szReal, 128);
		const DWORD uProxyLanguage = pfnLanguage(0x409, szProxy, 128);
		Check("VerLanguageNameW matches the real dll", uRealLanguage && uRealLanguage == uProxyLanguage && std::wstring_view(szReal) == szProxy);

		for (int i = 0; i < 1000; ++i)
			static_cast<void>(pfnSize(strFile.c_str(), &uProxyHandle));
		Check("repeated forwarded calls keep the stack balanced", pfnSize(strFile.c_str(), &uProxyHandle) == uRealSize);
	}

#if OMEGAWARE_ENGINE_UNITY
	void TestMono()
	{
		std::puts("mono");
		Check("initialize fails cleanly without a mono runtime", !OmegaWare::Mono::Initialize() && !OmegaWare::Mono::IsAvailable());
		Check("lookups return nothing while unavailable", !OmegaWare::Mono::GetImage() && !OmegaWare::Mono::GetClass("", "GameManager") && !OmegaWare::Mono::GetCompiledMethod("", "GameManager", "Update"));
		Check("static field read returns nothing while unavailable", !OmegaWare::Mono::GetStaticField<int>(nullptr, "value").has_value());
		Check("string helpers are safe while unavailable", OmegaWare::Mono::ToString(nullptr).empty() && !OmegaWare::Mono::NewString("omegaware"));
	}
#endif
}

extern "C" __declspec(dllexport) void* CreateInterface(const char* szName, int* pReturnCode)
{
	for (OmegaWare::Memory::stInterfaceRegistry* pRegistry = g_pInterfaceRegistry; pRegistry; pRegistry = pRegistry->pNext)
	{
		if (std::strcmp(pRegistry->szName, szName) == 0)
		{
			if (pReturnCode)
				*pReturnCode = 0;
			return pRegistry->pfnCreate();
		}
	}
	if (pReturnCode)
		*pReturnCode = 1;
	return nullptr;
}

int main()
{
	std::puts("omegaware framework tests");
	TestPatterns();
	TestScanning();
	TestSafety();
	TestNamePool();
	TestHooks();
	TestInterfaces();
	TestInputBlock();
	TestInputDevices();
	TestDirectInput7();
	TestGameInput();
	TestHeldInputRelease();
	TestLogFiles();
	TestCrashDump();
#if OMEGAWARE_ENGINE_UNITY
	TestMono();
#endif
#if OMEGAWARE_ENGINE_UNITY
	TestIl2Cpp();
#endif
	TestProxy();
	std::printf("%d checks, %d failure(s)\n", g_iChecks, g_iFailures);
	return g_iFailures == 0 ? 0 : 1;
}
