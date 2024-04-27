#include <Windows.h>
#include <filesystem>

#include "../OmegaWare Internal Framework/FrameworkConfig.h"
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
	const std::string Framework = FRAMEWORK_CODENAME;
	const std::string Game = FRAMEWORK_TARGET_GAME;
	const std::string Title = Framework + " (" + Game + ")";

	SetConsoleTitleA(Title.c_str());
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
		return DLL_NOT_FOUND;
	}

	bool bLoadLibrary = false;
	bool bManualMap = false;

	bool bBypass = false;

	if (ArgParser.FindValue("llib").bFound)
		bLoadLibrary = true;

	if (ArgParser.FindValue("mm").bFound)
		bManualMap = true;

	if (ArgParser.FindValue("bypass").bFound)
		bBypass = true;

	if (!bLoadLibrary && !bManualMap)
	{
		printf("No injection method specified\n");
		return NO_INJECTION_METHOD_SPECIFIED;
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
		return INJECTION_FAILED;
	}

	printf("Injection successful\n");
	return 0;
}