#include "pch.h"
#include "Hooks.hpp"

SDK::FVector Hooks::m_vecOriginalLocation = {};
SDK::FRotator Hooks::m_rotOriginalRotation = {};
bool Hooks::m_bFoundAimbotTarget = false;
SDK::ACH_BaseCop_C* Hooks::m_pATarget = nullptr;

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
    if (!pLocalPlayer)
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
		if (!pLocalPlayerControllerBase)
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

    m_vecULocalPlayerGetViewPointCallbacks.emplace_back([this](SDK::ULocalPlayer* _this, SDK::FMinimalViewInfo* OutViewInfo) { 
        if (!m_pAimbotToggle->GetValue())
            return;

        if (m_pSilentAimToggle->GetValue())
            OutViewInfo->Rotation = m_rotOriginalRotation;

        if (m_pMagicBulletToggle->GetValue())
            OutViewInfo->Location = m_vecOriginalLocation;

        return;
    });

    m_vecAPlayerControllerGetPlayerViewPointCallbacks.emplace_back([this](SDK::APlayerController* _this, SDK::FVector* out_Location, SDK::FRotator* out_Rotation) { 
        if (!m_pAimbotToggle->GetValue())
            return;
        
        m_vecOriginalLocation = *out_Location;
        m_rotOriginalRotation = *out_Rotation;

        if (m_bFoundAimbotTarget && GetAsyncKeyState(VK_RBUTTON)) {
            SDK::FRotator rotCurrent = *out_Rotation;
            SDK::FVector vecAimbotTargetLocation = m_pATarget->Mesh->GetSocketLocation(SDK::UKismetStringLibrary::Conv_StringToName(L"Head"));
            if (m_pMagicBulletToggle->GetValue() && m_pATarget) {
                *out_Location = vecAimbotTargetLocation;
            }
            SDK::FRotator rotGoal = SDK::UKismetMathLibrary::FindLookAtRotation(*out_Location, vecAimbotTargetLocation);
            *out_Rotation = (rotGoal - ((rotGoal - rotCurrent).GetNormalized() * 0.5f)).GetNormalized();
        }

        return;
    });

    return true;
}

void Hooks::Destroy() {
    if (UObjectProcessEvent_o) {
        MH_RemoveHook(UObjectProcessEvent_o);
    }

    if (UObjectProcessEventPlayer_o) {
        MH_RemoveHook(UObjectProcessEventPlayer_o);
    }

    if (ULocalPlayerGetViewPoint_o) {
        MH_RemoveHook(ULocalPlayerGetViewPoint_o);
    }

    if (APlayerControllerGetPlayerViewPoint_o) {
        MH_RemoveHook(APlayerControllerGetPlayerViewPoint_o);
    }
}

bool Hooks::SetupMenu() {
    Localization::AddToLocale("ENG", std::initializer_list<std::pair<size_t, std::string>>{
        { "AIMBOT_TOGGLE"Hashed, "Aimbot" },
		{ "SILENT_AIM_TOGGLE"Hashed, "Silent Aim" },
		{ "MAGIC_BULLET_TOGGLE"Hashed, "Magic Bullet" },
	});

    return true;
}

void Hooks::HandleMenu() {
    static std::once_flag onceflag;
	std::call_once(onceflag, [this]() {
        ElementBase* pFeaturesPage = Framework::menu->GetChild("HEADER_GROUP")->GetChild("BODY")->GetChild("ALL_FEATURES_PAGE");
        pFeaturesPage->AddElement(m_pAimbotToggle.get());
        pFeaturesPage->AddElement(m_pSilentAimToggle.get());
        pFeaturesPage->AddElement(m_pMagicBulletToggle.get());
    });

    m_pSilentAimToggle->SetVisible(m_pAimbotToggle->GetValue());
    m_pMagicBulletToggle->SetVisible(m_pAimbotToggle->GetValue());
}

void Hooks::Run() {
    SDK::UWorld* pGWorld = SDK::UWorld::GetWorld();
    if (!pGWorld)
        return;

    SDK::USBZWorldRuntime* pWorldRuntime = reinterpret_cast<SDK::USBZWorldRuntime*>(SDK::USBZWorldRuntime::GetWorldRuntime(pGWorld));
    if (!pWorldRuntime)
        return;

    if (m_pAimbotToggle->GetValue()) {
        UC::TArray<SDK::UObject*>& actors = pWorldRuntime->AllPawns->Objects;

        SDK::APlayerController* pPlayerController = Framework::unreal->GetPlayerController();

        SDK::FVector vecCameraLocation = pPlayerController->PlayerCameraManager->GetCameraLocation();
        std::vector<Unreal::ActorInfo> vecActors{};
        vecActors.reserve(actors.Num());
        for (int i = 0; i < actors.Num(); ++i){
            if (!actors.IsValidIndex(i))
                break;

            auto pActor = reinterpret_cast<SDK::AActor*>(actors[i]);
            if(!pActor)
                continue;

            auto eType = Framework::unreal->DetermineActorType(pActor);
            if (Framework::unreal->ShouldSkipActor(pActor, eType))
                continue;

            vecActors.emplace_back(Unreal::ActorInfo{
                .m_eType = eType,
                .m_flDistance = (pActor->K2_GetActorLocation() - vecCameraLocation).Magnitude(),
                .m_pActor = pActor
            });
        }    

        std::sort(vecActors.begin(), vecActors.end(), [](Unreal::ActorInfo& lhs, Unreal::ActorInfo& rhs) {
            if (lhs.m_flDistance == rhs.m_flDistance)
                rhs.m_flDistance += 0.001f;

            return lhs.m_flDistance > rhs.m_flDistance;
        });

        SDK::ACH_BaseCop_C* pATarget = nullptr;
        bool bDidFindTarget = false;
        for (Unreal::ActorInfo& infoActor : vecActors){
            auto pActor = infoActor.m_pActor;
            if (!pActor)
                continue;

            switch(infoActor.m_eType){
            case Unreal::EActorType::Guard:
            case Unreal::EActorType::Shield:
            case Unreal::EActorType::Cloaker:
            case Unreal::EActorType::Sniper:
            case Unreal::EActorType::Grenadier:
            case Unreal::EActorType::Taser:
            case Unreal::EActorType::Techie:
            case Unreal::EActorType::Dozer:
                pATarget = reinterpret_cast<SDK::ACH_BaseCop_C*>(pActor);
                bDidFindTarget = true;
                break;

            default:
                break;
            }
        }

        m_bFoundAimbotTarget = bDidFindTarget;
        m_pATarget = bDidFindTarget ? pATarget : nullptr;
    } else {
        m_bFoundAimbotTarget = false;
        m_pATarget = nullptr;
    }
}