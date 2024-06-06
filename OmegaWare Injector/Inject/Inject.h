#pragma once
#include <Windows.h>
#include <string>
#include <fstream>

#include <thread>
#include <chrono>

namespace Inject
{
	inline bool BypassVAC(HANDLE hProc)
	{
		HMODULE hNtdll = LoadLibraryA("ntdll.dll");
		if (!hNtdll)
			return false;

		LPVOID NtOpenFile = GetProcAddress(hNtdll, "NtOpenFile");

		char sOriginalBytes[5];
		memcpy(sOriginalBytes, NtOpenFile, 5);
		WriteProcessMemory(hProc, NtOpenFile, sOriginalBytes, 5, NULL);

		return true;
	}

	inline bool RestoreVAC(HANDLE hProc)
	{
		HMODULE hNtdll = LoadLibraryA("ntdll.dll");
		if (!hNtdll)
			return false;

		LPVOID NtOpenFile = GetProcAddress(hNtdll, "NtOpenFile");

		char sOriginalBytes[5];
		memcpy(sOriginalBytes, NtOpenFile, 5);
		WriteProcessMemory(hProc, NtOpenFile, sOriginalBytes, 0, NULL);

		return true;
	}

	inline bool LoadLib(HANDLE hProc, std::string DLLPath, bool bBypass = false)
	{
		if (bBypass)
			if (!BypassVAC(hProc))
			{
				printf("BypassVAC failed");
				return false;
			}

		LPVOID lpAllocatedMem = VirtualAllocEx(hProc, NULL, DLLPath.length(), MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
		if (!lpAllocatedMem)
		{
			printf("VirtualAllocEx failed: %d\n", GetLastError());
			return false;
		}

		WriteProcessMemory(hProc, lpAllocatedMem, DLLPath.c_str(), DLLPath.length(), NULL);

		HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");
		if (!hKernel32)
		{
			printf("GetModuleHandleA failed: %d\n", GetLastError());
			return false;
		}

		LPVOID lpLoadLibraryA = GetProcAddress(hKernel32, "LoadLibraryA");
		if (!lpLoadLibraryA)
		{
			printf("GetProcAddress failed: %d\n", GetLastError());
			return false;
		}

		HANDLE hThread = CreateRemoteThread(hProc, NULL, NULL, (LPTHREAD_START_ROUTINE)lpLoadLibraryA, lpAllocatedMem, NULL, NULL);
		if (!hThread)
		{
			printf("CreateRemoteThread failed: %d\n", GetLastError());
			return false;
		}
		CloseHandle(hThread);

		if (bBypass)
			if (!RestoreVAC(hProc))
			{
				printf("RestoreVAC failed");
				return false;
			}

		CloseHandle(hProc);

		return true;
	}

	using f_LoadLibraryA = HINSTANCE(WINAPI*)(const char* lpLibFilename);
	using f_GetProcAddress = UINT_PTR(WINAPI*)(HINSTANCE hModule, const char* lpProcName);
	using f_DLL_ENTRY_POINT = BOOL(WINAPI*)(void* hDll, DWORD dwReason, void* pReserved);

	struct MANUAL_MAPPING_DATA
	{
		f_LoadLibraryA		pLoadLibraryA;
		f_GetProcAddress	pGetProcAddress;
		HINSTANCE			hMod;
	};

#define RELOC_FLAG32(RelInfo) ((RelInfo >> 0x0C) == IMAGE_REL_BASED_HIGHLOW)
#define RELOC_FLAG64(RelInfo) ((RelInfo >> 0x0C) == IMAGE_REL_BASED_DIR64)

#ifdef _WIN64
#define RELOC_FLAG RELOC_FLAG64
#else
#define RELOC_FLAG RELOC_FLAG32
#endif

	inline void __stdcall Shellcode(MANUAL_MAPPING_DATA* pData)
	{
		if (!pData)
			return;

		BYTE* pBase = reinterpret_cast<BYTE*>(pData);
		auto* pOpt = &reinterpret_cast<IMAGE_NT_HEADERS*>(pBase + reinterpret_cast<IMAGE_DOS_HEADER*>(pData)->e_lfanew)->OptionalHeader;

		auto _LoadLibraryA = pData->pLoadLibraryA;
		auto _GetProcAddress = pData->pGetProcAddress;
		auto _DllMain = reinterpret_cast<f_DLL_ENTRY_POINT>(pBase + pOpt->AddressOfEntryPoint);

		BYTE* LocationDelta = pBase - pOpt->ImageBase;
		if (LocationDelta)
		{
			if (!pOpt->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size)
				return;

			auto* pRelocData = reinterpret_cast<IMAGE_BASE_RELOCATION*>(pBase + pOpt->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress);
			while (pRelocData->VirtualAddress)
			{
				UINT AmountOfEntries = (pRelocData->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(WORD);
				WORD* pRelativeInfo = reinterpret_cast<WORD*>(pRelocData + 1);

				for (UINT i = 0; i != AmountOfEntries; ++i, ++pRelativeInfo)
				{
					if (RELOC_FLAG(*pRelativeInfo))
					{
						UINT_PTR* pPatch = reinterpret_cast<UINT_PTR*>(pBase + pRelocData->VirtualAddress + ((*pRelativeInfo) & 0xFFF));
						*pPatch += reinterpret_cast<UINT_PTR>(LocationDelta);
					}
				}
				pRelocData = reinterpret_cast<IMAGE_BASE_RELOCATION*>(reinterpret_cast<BYTE*>(pRelocData) + pRelocData->SizeOfBlock);
			}
		}

		if (pOpt->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size)
		{
			auto* pImportDescr = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(pBase + pOpt->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
			while (pImportDescr->Name)
			{
				char* szMod = reinterpret_cast<char*>(pBase + pImportDescr->Name);
				HINSTANCE hDll = _LoadLibraryA(szMod);

				ULONG_PTR* pThunkRef = reinterpret_cast<ULONG_PTR*>(pBase + pImportDescr->OriginalFirstThunk);
				ULONG_PTR* pFuncRef = reinterpret_cast<ULONG_PTR*>(pBase + pImportDescr->FirstThunk);

				if (!pThunkRef)
					pThunkRef = pFuncRef;

				for (; *pThunkRef; ++pThunkRef, ++pFuncRef)
				{
					if (IMAGE_SNAP_BY_ORDINAL(*pThunkRef))
					{
						*pFuncRef = _GetProcAddress(hDll, reinterpret_cast<char*>(*pThunkRef & 0xFFFF));
					}
					else
					{
						auto* pImport = reinterpret_cast<IMAGE_IMPORT_BY_NAME*>(pBase + (*pThunkRef));
						*pFuncRef = _GetProcAddress(hDll, pImport->Name);
					}
				}
				++pImportDescr;
			}
		}

		if (pOpt->DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].Size)
		{
			auto* pTLS = reinterpret_cast<IMAGE_TLS_DIRECTORY*>(pBase + pOpt->DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress);
			auto* pCallback = reinterpret_cast<PIMAGE_TLS_CALLBACK*>(pTLS->AddressOfCallBacks);
			for (; pCallback && *pCallback; ++pCallback)
				(*pCallback)(pBase, DLL_PROCESS_ATTACH, nullptr);
		}

		_DllMain(pBase, DLL_PROCESS_ATTACH, nullptr);

		pData->hMod = reinterpret_cast<HINSTANCE>(pBase);
	}

	inline bool ManualMap(HANDLE hProc, std::string DLLPath)
	{
		BYTE* pSrcData = nullptr;
		IMAGE_NT_HEADERS* pOldNtHeaders = nullptr;
		IMAGE_OPTIONAL_HEADER* pOldOptHeader = nullptr;
		IMAGE_FILE_HEADER* pOldFileHeader = nullptr;
		BYTE* pTargetBase = nullptr;

		std::ifstream DLL(DLLPath, std::ios::binary | std::ios::ate);
		if (!DLL.good())
		{
			printf("DLL fstream failed\n");
			DLL.close();
			return false;
		}

		std::streampos DLLSize = DLL.tellg();
		if (DLLSize < 0x1000)
		{
			printf("DLL file size is invalid\n");
			DLL.close();
			return false;
		}

		pSrcData = new BYTE[static_cast<UINT_PTR>(DLLSize)];
		if (!pSrcData)
		{
			printf("pSrcData allocation failed\n");
			DLL.close();
			return false;
		}

		DLL.seekg(0, std::ios::beg);
		DLL.read(reinterpret_cast<char*>(pSrcData), DLLSize);
		DLL.close();

		if (reinterpret_cast<IMAGE_DOS_HEADER*>(pSrcData)->e_magic != IMAGE_DOS_SIGNATURE)
		{
			printf("Invalid DOS signature\n");
			delete[] pSrcData;
			return false;
		}

		pOldNtHeaders = reinterpret_cast<IMAGE_NT_HEADERS*>(pSrcData + reinterpret_cast<IMAGE_DOS_HEADER*>(pSrcData)->e_lfanew);
		if (pOldNtHeaders->Signature != IMAGE_NT_SIGNATURE)
		{
			printf("Invalid NT signature\n");
			delete[] pSrcData;
			return false;
		}

		pOldOptHeader = &pOldNtHeaders->OptionalHeader;
		pOldFileHeader = &pOldNtHeaders->FileHeader;

#ifdef _WIN64
		if (pOldFileHeader->Machine != IMAGE_FILE_MACHINE_AMD64)
		{
			printf("Invalid machine type, process is not x64\n");
			delete[] pSrcData;
			return false;
		}
#endif

#ifdef _WIN32
		if (pOldFileHeader->Machine != IMAGE_FILE_MACHINE_I386)
		{
			printf("Invalid machine type, process is not x86\n");
			delete[] pSrcData;
			return false;
		}
#endif

		pTargetBase = reinterpret_cast<BYTE*>(VirtualAllocEx(hProc, reinterpret_cast<void*>(pOldOptHeader->ImageBase), pOldOptHeader->SizeOfImage, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
		if (!pTargetBase)
		{
			pTargetBase = reinterpret_cast<BYTE*>(VirtualAllocEx(hProc, nullptr, pOldOptHeader->SizeOfImage, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
			if (!pTargetBase)
			{
				printf("VirtualAllocEx failed: %d\n", GetLastError());
				delete[] pSrcData;
				return false;
			}
		}
		
		MANUAL_MAPPING_DATA Data{ 0 };
		Data.pLoadLibraryA = LoadLibraryA;
		Data.pGetProcAddress = reinterpret_cast<f_GetProcAddress>(GetProcAddress);

		_IMAGE_SECTION_HEADER* pSectionHeader = IMAGE_FIRST_SECTION(pOldNtHeaders);
		for (UINT i = 0; i != pOldFileHeader->NumberOfSections; ++i, ++pSectionHeader)
		{
			if (pSectionHeader->SizeOfRawData)
			{
				if (!WriteProcessMemory(hProc, pTargetBase + pSectionHeader->VirtualAddress, pSrcData + pSectionHeader->PointerToRawData, pSectionHeader->SizeOfRawData, nullptr))
				{
					printf("WriteProcessMemory failed: %d\n", GetLastError());
					delete[] pSrcData;
					VirtualFreeEx(hProc, pTargetBase, 0, MEM_RELEASE);
					return false;
				}
			}
		}

		memcpy_s(pSrcData, static_cast<UINT_PTR>(DLLSize), &Data, sizeof(Data));
		if (!WriteProcessMemory(hProc, pTargetBase, pSrcData, 0x1000, nullptr))
		{
			printf("WriteProcessMemory failed: %d\n", GetLastError());
			delete[] pSrcData;
			VirtualFreeEx(hProc, pTargetBase, 0, MEM_RELEASE);
			return false;
		}

		delete[] pSrcData;

		void* pShellCode = VirtualAllocEx(hProc, nullptr, 0x1000, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
		if (!pShellCode)
		{
			printf("VirtualAllocEx failed: %d\n", GetLastError());
			VirtualFreeEx(hProc, pTargetBase, 0, MEM_RELEASE);
			return false;
		}

		WriteProcessMemory(hProc, pShellCode, Shellcode, 0x1000, nullptr);

		HANDLE hThread = CreateRemoteThread(hProc, nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(pShellCode), pTargetBase, 0, nullptr);
		if (!hThread)
		{
			printf("CreateRemoteThread failed: %d\n", GetLastError());
			VirtualFreeEx(hProc, pTargetBase, 0, MEM_RELEASE);
			VirtualFreeEx(hProc, pShellCode, 0, MEM_RELEASE);
			return false;
		}
		CloseHandle(hThread);

		HINSTANCE hCheck = NULL;
		while (!hCheck)
		{
			MANUAL_MAPPING_DATA DataCheck{ 0 };
			if (!ReadProcessMemory(hProc, pTargetBase, &DataCheck, sizeof(DataCheck), nullptr))
				break;

			hCheck = DataCheck.hMod;
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		}

		VirtualFreeEx(hProc, pShellCode, 0, MEM_RELEASE);

		return true;
	}
}