#include "pch.h"
#include "Hooks.hpp"

bool Hooks::Setup() {
    SDK::UWorld* pGWorld = SDK::UWorld::GetWorld();
	if (!pGWorld)
		return false;

    if (!UObjectProcessEvent_o)
    {
        auto pFn = reinterpret_cast<void*>(SDK::InSDKUtils::GetVirtualFunction<UObjectProcessEvent_t>(pGWorld, SDK::Offsets::ProcessEventIdx)); 
        MH_STATUS status = MH_CreateHook(pFn, reinterpret_cast<void*>(&UObjectProcessEvent_hk), reinterpret_cast<void**>(&UObjectProcessEvent_o));
        if (status == MH_OK) {
            Utils::LogHook("UObjectProcessEvent", MH_EnableHook(pFn));
        } else {
            Utils::LogHook("UObjectProcessEvent", status);
            return false;
        }
    }

    SDK::ULocalPlayer* pLocalPlayer = Framework::unreal->GetLocalPlayer();
    if (!IsValidObjectPtr(pLocalPlayer))
        return false;

    if (!ULocalPlayerGetViewPoint_o)
    {
        auto pFn = reinterpret_cast<void*>(SDK::InSDKUtils::GetVirtualFunction<ULocalPlayerGetViewPoint_t>(pLocalPlayer, 0x50));
        MH_STATUS status = MH_CreateHook(pFn, reinterpret_cast<void*>(&ULocalPlayerGetViewPoint_hk), reinterpret_cast<void**>(&ULocalPlayerGetViewPoint_o));
        if (status == MH_OK) {
            Utils::LogHook("ULocalPlayerGetViewPoint", MH_EnableHook(pFn));
        } else {
            Utils::LogHook("ULocalPlayerGetViewPoint", status);
            return false;
        }
    }

    SDK::APlayerController* pLocalPlayerControllerBase = pLocalPlayer->PlayerController;
		if (!IsValidObjectPtr(pLocalPlayerControllerBase))
        return false;

    if (!APlayerControllerGetPlayerViewPoint_o)
    {
        auto pFn = reinterpret_cast<void*>(SDK::InSDKUtils::GetVirtualFunction<APlayerControllerGetPlayerViewPoint_t>(pLocalPlayerControllerBase, 0xED));
        MH_STATUS status = MH_CreateHook(pFn, reinterpret_cast<void*>(&APlayerControllerGetPlayerViewPoint_hk), reinterpret_cast<void**>(&APlayerControllerGetPlayerViewPoint_o));
        if (status == MH_OK) {
            Utils::LogHook("APlayerControllerGetPlayerViewPoint", MH_EnableHook(pFn));
        } else {
            Utils::LogHook("APlayerControllerGetPlayerViewPoint", status);
            return false;
        }
    }

    if(!UObjectProcessEventPlayer_o)
    {
        auto pFn = reinterpret_cast<void*>(SDK::InSDKUtils::GetVirtualFunction<UObjectProcessEvent_t>(pLocalPlayerControllerBase, SDK::Offsets::ProcessEventIdx)); 
        MH_STATUS status = MH_CreateHook(pFn, reinterpret_cast<void*>(&UObjectProcessEventPlayer_hk), reinterpret_cast<void**>(&UObjectProcessEventPlayer_o));
        if (status == MH_OK) {
            Utils::LogHook("UObjectProcessEventPlayer", MH_EnableHook(pFn));
        } else {
            Utils::LogHook("UObjectProcessEventPlayer", status);
            return false;
        }
    }

    return true;
}

void Hooks::Destroy() {
    if (UObjectProcessEvent_o) {
        MH_DisableHook(UObjectProcessEvent_o);
        UObjectProcessEvent_o = nullptr;
    }

    if (UObjectProcessEventPlayer_o) {
        MH_DisableHook(UObjectProcessEventPlayer_o);
        UObjectProcessEventPlayer_o = nullptr;
    }

    if (ULocalPlayerGetViewPoint_o) {
        MH_DisableHook(ULocalPlayerGetViewPoint_o);
        ULocalPlayerGetViewPoint_o = nullptr;
    }

    if (APlayerControllerGetPlayerViewPoint_o) {
        MH_DisableHook(APlayerControllerGetPlayerViewPoint_o);
        APlayerControllerGetPlayerViewPoint_o = nullptr;
    }
}