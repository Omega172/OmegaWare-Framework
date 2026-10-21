#include "Core/Framework.hpp"

BOOL APIENTRY DllMain(HMODULE hModule, DWORD uReason, LPVOID)
{
	if (uReason == DLL_PROCESS_ATTACH)
	{
		DisableThreadLibraryCalls(hModule);
		if (OMEGAWARE_PROXY && GetEnvironmentVariableW(L"OMEGAWARE_DISABLE", nullptr, 0) > 0)
			return TRUE;
		OmegaWare::Start(hModule);
	}
	return TRUE;
}
