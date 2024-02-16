#include <Windows.h>
#include <filesystem>

#include "../OmegaWare Framework/pch.h"
#include "Parse/Parse.h"
#include "Utils/Utils.h"
#include "Inject/Inject.h"

// This injector is made to be used with the OmegaWare Framework it does not support taking in a process name as an argument,
// it assumes you have the process name hardcoded in the FRAMEWORK_TARGET_PROCESS macro in pch.h

enum ERROR_CODES
{
	DLL_NOT_FOUND = 1,
	NO_INJECTION_METHOD_SPECIFIED = 2,
	INJECTION_FAILED = 3
};

int main(int argc, char** argv, char** envp)
{
	SetConsoleTitleA(Cheat::Title.c_str());
	Parse ArgParser(argc, argv);

	if (argc < 2 || ArgParser.FindValue("--help").bFound || ArgParser.FindValue("-h").bFound)
	{
		printf("TARGET PROCESS: %s\n", FRAMEWORK_TARGET_PROCESS);
		printf("Usage: %s DLL [options]\n", argv[0]);
		printf("Options:\n");
		printf("  -llib             LoadLibrary\n");
		printf("  -mm               ManualMap\n");
		printf("  -bypass           Bypass VAC\n");
		return 0;
	}

	std::filesystem::path DLLPath = ArgParser.GetArg(1);
	if (!std::filesystem::exists(DLLPath))
	{
		printf("Error: DLL not found at %ws", DLLPath.c_str());
		return 1;
	}

	bool bLoadLibrary = false;
	bool bManualMap = false;

	bool bBypass = false;

	Parse::ParseData LLIB = ArgParser.FindValue("llib");
	if (LLIB.bFound)
		bLoadLibrary = true;

	Parse::ParseData MM = ArgParser.FindValue("mm");
	if (MM.bFound)
		bManualMap = true;

	Parse::ParseData BYPASS = ArgParser.FindValue("bypass");
	if (BYPASS.bFound)
		bBypass = true;

	if (!bLoadLibrary && !bManualMap)
	{
		printf("No injection method specified\n");
		return 2;
	}

	if (bLoadLibrary && bManualMap)
	{
		printf("Cannot use both -llib and -mm\n");
		printf("Defaulting to -llib\n");

		bManualMap = false;
	}

	if (bManualMap && bBypass)
	{
		printf("Cannot use -bypass with -mm\n");
		printf("Defaulting to -llib\n");

		bLoadLibrary = true;
		bManualMap = false;
	}

	DWORD dwPID = Utils::GetProcId(FRAMEWORK_TARGET_PROCESS);
	printf("PID: %d\n", dwPID);

	HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwPID);
	printf("hProc: %p\n", hProc);

	bool bReturnVal = false;
	if (bLoadLibrary)
		bReturnVal = Inject::LoadLib(hProc, DLLPath.string(), bBypass);
		
	if (bManualMap)
		bReturnVal = Inject::ManualMap(hProc, DLLPath.string());

	if (!bReturnVal)
	{
		printf("Injection failed\n");
		return 3;
	}

	printf("Injection successful\n");
	return 0;
}