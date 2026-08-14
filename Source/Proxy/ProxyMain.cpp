// Export forwarding for the Proxy target when built as version.dll (see proxy_dll_name and the
// PROXY define in xmake.lua - Proxy compiles the same sources as Internal, so this file only
// adds what Internal doesn't already have: the forwarded exports themselves). Rather than
// linker-level export forwarding to a renamed copy of the system DLL - which needs that copy to
// exist somewhere and breaks if it goes missing or the OS updates the real one - this loads the
// genuine version.dll from the real System32 path at runtime and resolves each export via
// GetProcAddress, then forwards calls through typed function pointers.
//
// ProxyInitRealVersionDll() is called from Internal/dllmain.cpp's DllMain (under #ifdef PROXY)
// before anything else runs, since the forwarded exports need to work immediately.
//
// Signatures below are the standard documented winver.h prototypes, reproduced locally rather
// than pulled in via #include <winver.h> - that header declares these as dllimport, which
// conflicts with also defining them as this DLL's own exports here.
// GetFileVersionInfoByHandle is undocumented by Microsoft; its signature here matches the one
// published on Microsoft Learn (BOOL GetFileVersionInfoByHandle(DWORD, HANDLE, LPVOID*, PDWORD)),
// which also notes it has no import library and must be resolved via GetProcAddress - exactly
// what we're doing for every export here anyway.

#define NOMINMAX
#include <Windows.h>
#include <string>

#include "Proxy.hpp"

namespace
{
	HMODULE g_hRealVersion = nullptr;

	using GetFileVersionInfoA_t = BOOL(WINAPI*)(LPCSTR, DWORD, DWORD, LPVOID);
	using GetFileVersionInfoByHandle_t = BOOL(WINAPI*)(DWORD, HANDLE, LPVOID*, PDWORD);
	using GetFileVersionInfoExA_t = BOOL(WINAPI*)(DWORD, LPCSTR, DWORD, DWORD, LPVOID);
	using GetFileVersionInfoExW_t = BOOL(WINAPI*)(DWORD, LPCWSTR, DWORD, DWORD, LPVOID);
	using GetFileVersionInfoSizeA_t = DWORD(WINAPI*)(LPCSTR, LPDWORD);
	using GetFileVersionInfoSizeExA_t = DWORD(WINAPI*)(DWORD, LPCSTR, LPDWORD);
	using GetFileVersionInfoSizeExW_t = DWORD(WINAPI*)(DWORD, LPCWSTR, LPDWORD);
	using GetFileVersionInfoSizeW_t = DWORD(WINAPI*)(LPCWSTR, LPDWORD);
	using GetFileVersionInfoW_t = BOOL(WINAPI*)(LPCWSTR, DWORD, DWORD, LPVOID);
	using VerFindFileA_t = DWORD(WINAPI*)(DWORD, LPCSTR, LPCSTR, LPCSTR, LPSTR, PUINT, LPSTR, PUINT);
	using VerFindFileW_t = DWORD(WINAPI*)(DWORD, LPCWSTR, LPCWSTR, LPCWSTR, LPWSTR, PUINT, LPWSTR, PUINT);
	using VerInstallFileA_t = DWORD(WINAPI*)(DWORD, LPCSTR, LPCSTR, LPCSTR, LPCSTR, LPCSTR, LPSTR, PUINT);
	using VerInstallFileW_t = DWORD(WINAPI*)(DWORD, LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR, LPWSTR, PUINT);
	using VerLanguageNameA_t = DWORD(WINAPI*)(DWORD, LPSTR, DWORD);
	using VerLanguageNameW_t = DWORD(WINAPI*)(DWORD, LPWSTR, DWORD);
	using VerQueryValueA_t = BOOL(WINAPI*)(LPCVOID, LPCSTR, LPVOID*, PUINT);
	using VerQueryValueW_t = BOOL(WINAPI*)(LPCVOID, LPCWSTR, LPVOID*, PUINT);

	GetFileVersionInfoA_t p_GetFileVersionInfoA = nullptr;
	GetFileVersionInfoByHandle_t p_GetFileVersionInfoByHandle = nullptr;
	GetFileVersionInfoExA_t p_GetFileVersionInfoExA = nullptr;
	GetFileVersionInfoExW_t p_GetFileVersionInfoExW = nullptr;
	GetFileVersionInfoSizeA_t p_GetFileVersionInfoSizeA = nullptr;
	GetFileVersionInfoSizeExA_t p_GetFileVersionInfoSizeExA = nullptr;
	GetFileVersionInfoSizeExW_t p_GetFileVersionInfoSizeExW = nullptr;
	GetFileVersionInfoSizeW_t p_GetFileVersionInfoSizeW = nullptr;
	GetFileVersionInfoW_t p_GetFileVersionInfoW = nullptr;
	VerFindFileA_t p_VerFindFileA = nullptr;
	VerFindFileW_t p_VerFindFileW = nullptr;
	VerInstallFileA_t p_VerInstallFileA = nullptr;
	VerInstallFileW_t p_VerInstallFileW = nullptr;
	VerLanguageNameA_t p_VerLanguageNameA = nullptr;
	VerLanguageNameW_t p_VerLanguageNameW = nullptr;
	VerQueryValueA_t p_VerQueryValueA = nullptr;
	VerQueryValueW_t p_VerQueryValueW = nullptr;

	template<typename Fn_t>
	bool Resolve(Fn_t& fnOut, const char* szName)
	{
		fnOut = reinterpret_cast<Fn_t>(GetProcAddress(g_hRealVersion, szName));
		return fnOut != nullptr;
	}

}

bool ProxyInitRealVersionDll()
{
	wchar_t szSystemDir[MAX_PATH]{};
	if (!GetSystemDirectoryW(szSystemDir, MAX_PATH))
		return false;

	std::wstring sPath = szSystemDir;
	sPath += L"\\version.dll";

	g_hRealVersion = LoadLibraryW(sPath.c_str());
	if (!g_hRealVersion)
		return false;

	bool bOk = true;
	bOk &= Resolve(p_GetFileVersionInfoA, "GetFileVersionInfoA");
	bOk &= Resolve(p_GetFileVersionInfoByHandle, "GetFileVersionInfoByHandle");
	bOk &= Resolve(p_GetFileVersionInfoExA, "GetFileVersionInfoExA");
	bOk &= Resolve(p_GetFileVersionInfoExW, "GetFileVersionInfoExW");
	bOk &= Resolve(p_GetFileVersionInfoSizeA, "GetFileVersionInfoSizeA");
	bOk &= Resolve(p_GetFileVersionInfoSizeExA, "GetFileVersionInfoSizeExA");
	bOk &= Resolve(p_GetFileVersionInfoSizeExW, "GetFileVersionInfoSizeExW");
	bOk &= Resolve(p_GetFileVersionInfoSizeW, "GetFileVersionInfoSizeW");
	bOk &= Resolve(p_GetFileVersionInfoW, "GetFileVersionInfoW");
	bOk &= Resolve(p_VerFindFileA, "VerFindFileA");
	bOk &= Resolve(p_VerFindFileW, "VerFindFileW");
	bOk &= Resolve(p_VerInstallFileA, "VerInstallFileA");
	bOk &= Resolve(p_VerInstallFileW, "VerInstallFileW");
	bOk &= Resolve(p_VerLanguageNameA, "VerLanguageNameA");
	bOk &= Resolve(p_VerLanguageNameW, "VerLanguageNameW");
	bOk &= Resolve(p_VerQueryValueA, "VerQueryValueA");
	bOk &= Resolve(p_VerQueryValueW, "VerQueryValueW");

	return bOk;
}

extern "C" {

BOOL WINAPI GetFileVersionInfoA(LPCSTR lptstrFilename, DWORD dwHandle, DWORD dwLen, LPVOID lpData)
{
	return p_GetFileVersionInfoA(lptstrFilename, dwHandle, dwLen, lpData);
}

BOOL WINAPI GetFileVersionInfoByHandle(DWORD dwFlags, HANDLE hFile, LPVOID* lplpData, PDWORD pdwLen)
{
	return p_GetFileVersionInfoByHandle(dwFlags, hFile, lplpData, pdwLen);
}

BOOL WINAPI GetFileVersionInfoExA(DWORD dwFlags, LPCSTR lpwstrFilename, DWORD dwHandle, DWORD dwLen, LPVOID lpData)
{
	return p_GetFileVersionInfoExA(dwFlags, lpwstrFilename, dwHandle, dwLen, lpData);
}

BOOL WINAPI GetFileVersionInfoExW(DWORD dwFlags, LPCWSTR lpwstrFilename, DWORD dwHandle, DWORD dwLen, LPVOID lpData)
{
	return p_GetFileVersionInfoExW(dwFlags, lpwstrFilename, dwHandle, dwLen, lpData);
}

DWORD WINAPI GetFileVersionInfoSizeA(LPCSTR lptstrFilename, LPDWORD lpdwHandle)
{
	return p_GetFileVersionInfoSizeA(lptstrFilename, lpdwHandle);
}

DWORD WINAPI GetFileVersionInfoSizeExA(DWORD dwFlags, LPCSTR lpwstrFilename, LPDWORD lpdwHandle)
{
	return p_GetFileVersionInfoSizeExA(dwFlags, lpwstrFilename, lpdwHandle);
}

DWORD WINAPI GetFileVersionInfoSizeExW(DWORD dwFlags, LPCWSTR lpwstrFilename, LPDWORD lpdwHandle)
{
	return p_GetFileVersionInfoSizeExW(dwFlags, lpwstrFilename, lpdwHandle);
}

DWORD WINAPI GetFileVersionInfoSizeW(LPCWSTR lptstrFilename, LPDWORD lpdwHandle)
{
	return p_GetFileVersionInfoSizeW(lptstrFilename, lpdwHandle);
}

BOOL WINAPI GetFileVersionInfoW(LPCWSTR lptstrFilename, DWORD dwHandle, DWORD dwLen, LPVOID lpData)
{
	return p_GetFileVersionInfoW(lptstrFilename, dwHandle, dwLen, lpData);
}

DWORD WINAPI VerFindFileA(DWORD uFlags, LPCSTR szFileName, LPCSTR szWinDir, LPCSTR szAppDir, LPSTR szCurDir, PUINT lpuCurDirLen, LPSTR szDestDir, PUINT lpuDestDirLen)
{
	return p_VerFindFileA(uFlags, szFileName, szWinDir, szAppDir, szCurDir, lpuCurDirLen, szDestDir, lpuDestDirLen);
}

DWORD WINAPI VerFindFileW(DWORD uFlags, LPCWSTR szFileName, LPCWSTR szWinDir, LPCWSTR szAppDir, LPWSTR szCurDir, PUINT lpuCurDirLen, LPWSTR szDestDir, PUINT lpuDestDirLen)
{
	return p_VerFindFileW(uFlags, szFileName, szWinDir, szAppDir, szCurDir, lpuCurDirLen, szDestDir, lpuDestDirLen);
}

DWORD WINAPI VerInstallFileA(DWORD uFlags, LPCSTR szSrcFileName, LPCSTR szDestFileName, LPCSTR szSrcDir, LPCSTR szDestDir, LPCSTR szCurDir, LPSTR szTmpFile, PUINT lpuTmpFileLen)
{
	return p_VerInstallFileA(uFlags, szSrcFileName, szDestFileName, szSrcDir, szDestDir, szCurDir, szTmpFile, lpuTmpFileLen);
}

DWORD WINAPI VerInstallFileW(DWORD uFlags, LPCWSTR szSrcFileName, LPCWSTR szDestFileName, LPCWSTR szSrcDir, LPCWSTR szDestDir, LPCWSTR szCurDir, LPWSTR szTmpFile, PUINT lpuTmpFileLen)
{
	return p_VerInstallFileW(uFlags, szSrcFileName, szDestFileName, szSrcDir, szDestDir, szCurDir, szTmpFile, lpuTmpFileLen);
}

DWORD WINAPI VerLanguageNameA(DWORD wLang, LPSTR szLang, DWORD cchLang)
{
	return p_VerLanguageNameA(wLang, szLang, cchLang);
}

DWORD WINAPI VerLanguageNameW(DWORD wLang, LPWSTR szLang, DWORD cchLang)
{
	return p_VerLanguageNameW(wLang, szLang, cchLang);
}

BOOL WINAPI VerQueryValueA(LPCVOID pBlock, LPCSTR lpSubBlock, LPVOID* lplpBuffer, PUINT puLen)
{
	return p_VerQueryValueA(pBlock, lpSubBlock, lplpBuffer, puLen);
}

BOOL WINAPI VerQueryValueW(LPCVOID pBlock, LPCWSTR lpSubBlock, LPVOID* lplpBuffer, PUINT puLen)
{
	return p_VerQueryValueW(pBlock, lpSubBlock, lplpBuffer, puLen);
}

} // extern "C"
