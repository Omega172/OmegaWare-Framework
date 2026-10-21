#include "Core/CrashDump.hpp"
#include "Core/Framework.hpp"
#include "Core/Log.hpp"

#include "OmegaRenderHook/Core/Guarded.hpp"

#include <Windows.h>
#include <DbgHelp.h>

#include <atomic>
#include <chrono>
#include <format>
#include <mutex>

namespace OmegaWare::CrashDump
{
	namespace
	{
		constexpr int kMaxDumps = 5;

		using MiniDumpWriteDumpFn = BOOL(WINAPI*)(HANDLE, DWORD, HANDLE, MINIDUMP_TYPE, PMINIDUMP_EXCEPTION_INFORMATION, PMINIDUMP_USER_STREAM_INFORMATION, PMINIDUMP_CALLBACK_INFORMATION);

		std::mutex g_mtxDump;
		std::filesystem::path g_pathDirectory;
		MiniDumpWriteDumpFn g_pfnWriteDump = nullptr;
		std::atomic<int> g_iDumpsWritten = 0;

		void WriteDump(const void* pExceptionPointers)
		{
			std::scoped_lock lckDump(g_mtxDump);
			if (!g_pfnWriteDump || g_iDumpsWritten >= kMaxDumps)
				return;

			auto* pPointers = static_cast<EXCEPTION_POINTERS*>(const_cast<void*>(pExceptionPointers));
			std::error_code ecDirectory;
			std::filesystem::create_directories(g_pathDirectory, ecDirectory);
			const auto tpNow = std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now());
			const std::filesystem::path pathDump = g_pathDirectory / std::format("{}-{:%Y%m%d-%H%M%S}-{:08X}.dmp", kName, tpNow, pPointers->ExceptionRecord->ExceptionCode);

			const HANDLE hFile = CreateFileW(pathDump.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
			if (hFile == INVALID_HANDLE_VALUE)
				return;
			MINIDUMP_EXCEPTION_INFORMATION stException{ GetCurrentThreadId(), pPointers, FALSE };
			const auto eType = static_cast<MINIDUMP_TYPE>(MiniDumpWithIndirectlyReferencedMemory | MiniDumpScanMemory | MiniDumpWithThreadInfo);
			const BOOL bWritten = g_pfnWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, eType, &stException, nullptr, nullptr);
			CloseHandle(hFile);
			if (bWritten)
			{
				++g_iDumpsWritten;
				LogError("fault 0x{:08X} at {}, crash dump written to {}", pPointers->ExceptionRecord->ExceptionCode, pPointers->ExceptionRecord->ExceptionAddress, pathDump.string());
			}
			else
				DeleteFileW(pathDump.c_str());
		}
	}

	void Install(const std::filesystem::path& pathDirectory)
	{
		std::scoped_lock lckDump(g_mtxDump);
		g_pathDirectory = pathDirectory;
		if (const HMODULE hDbgHelp = LoadLibraryExW(L"dbghelp.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32))
			g_pfnWriteDump = reinterpret_cast<MiniDumpWriteDumpFn>(GetProcAddress(hDbgHelp, "MiniDumpWriteDump"));
		if (!g_pfnWriteDump)
		{
			LogWarning("dbghelp.dll is unavailable, crash dumps are disabled");
			return;
		}
		OmegaRenderHook::SetFaultObserver(&WriteDump);
	}

	void Uninstall()
	{
		OmegaRenderHook::SetFaultObserver(nullptr);
	}
}
