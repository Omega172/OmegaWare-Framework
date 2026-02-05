#pragma once
#include "pch.h"


inline bool FrameworkUnrealInit()
{
	SDK::UWorld* pGWorld = SDK::UWorld::GetWorld();
	if (!pGWorld)
		Utils::LogError("Waiting for GWorld to initalize");

	while (!pGWorld)
		continue;

	return true;
}

class Unreal
{
public:
	// Shortcut functions to get pointers to important classes used for many things
	static SDK::UKismetMathLibrary* GetMathLibrary() { return reinterpret_cast<SDK::UKismetMathLibrary*>(SDK::UKismetMathLibrary::StaticClass()); }
	static SDK::UKismetSystemLibrary* GetSystemLibrary() { return reinterpret_cast<SDK::UKismetSystemLibrary*>(SDK::UKismetSystemLibrary::StaticClass()); }
	static SDK::UGameplayStatics* GetGameplayStatics() { return reinterpret_cast<SDK::UGameplayStatics*>(SDK::UGameplayStatics::StaticClass()); }
	static SDK::UKismetStringLibrary* GetStringLibrary() { return reinterpret_cast<SDK::UKismetStringLibrary*>(SDK::UKismetStringLibrary::StaticClass()); }

	// These functions are to make getting pointers to important classes and objects easier and cleaner
	static SDK::AGameStateBase* GetGameStateBase()
	{
		SDK::UWorld* pGWorld = SDK::UWorld::GetWorld();
		if (!pGWorld)
			return nullptr;

		SDK::AGameStateBase* pGameState = pGWorld->GameState;
		if (!IsValidObjectPtr(pGameState))
			return nullptr;

		return pGameState;
	}
	static SDK::UGameInstance* GetGameInstance()
	{
		SDK::UWorld* pGWorld = SDK::UWorld::GetWorld();
		if (!pGWorld)
			return nullptr;

		SDK::UGameInstance* pGameInstance = pGWorld->OwningGameInstance;
		if (!IsValidObjectPtr(pGameInstance))
			return nullptr;

		return pGameInstance;
	}

	static SDK::ULocalPlayer* GetLocalPlayer(int index = 0)
	{
		SDK::UGameInstance* pGameInstance = GetGameInstance();
		if (!IsValidObjectPtr(pGameInstance))
			return nullptr;

		SDK::ULocalPlayer* pLocalPlayer = pGameInstance->LocalPlayers[index];
		if (!IsValidObjectPtr(pLocalPlayer))
			return nullptr;

		return pLocalPlayer;
	}
	static SDK::UGameViewportClient* GetViewportClient()
	{
		SDK::ULocalPlayer* pLocalPlayer = GetLocalPlayer();
		if (!IsValidObjectPtr(pLocalPlayer))
			return nullptr;

		SDK::UGameViewportClient* pViewportClient = pLocalPlayer->ViewportClient;
		if (!IsValidObjectPtr(pViewportClient))
			return nullptr;

		return pViewportClient;
	}
	static SDK::APlayerController* GetPlayerController()
	{
		SDK::ULocalPlayer* LocalPlayer = GetLocalPlayer();
		if (!IsValidObjectPtr(LocalPlayer))
			return nullptr;

		SDK::APlayerController* pPlayerController = LocalPlayer->PlayerController;
		if (!IsValidObjectPtr(pPlayerController))
			return nullptr;

		return pPlayerController;
	}
	static SDK::APawn* GetAcknowledgedPawn()
	{
		SDK::APlayerController* pPlayerController = GetPlayerController();
		if (!IsValidObjectPtr(pPlayerController))
			return nullptr;

		SDK::APawn* pAcknowledgedPawn = pPlayerController->AcknowledgedPawn;
		if (!IsValidObjectPtr(pAcknowledgedPawn))
			return nullptr;

		return pAcknowledgedPawn;
	}

	static SDK::APlayerCameraManager* GetPlayerCameraManager()
	{
		SDK::APlayerController* pPlayerController = GetPlayerController();
		if (!IsValidObjectPtr(pPlayerController))
			return nullptr;

		SDK::APlayerCameraManager* pPlayerCameraManager = pPlayerController->PlayerCameraManager;
		if (!IsValidObjectPtr(pPlayerCameraManager))
			return nullptr;

		return pPlayerCameraManager;
	}

	static bool WorldToScreen(SDK::FVector in, SDK::FVector2D& out, bool relative = false)
	{
		SDK::APlayerController* pPlayerController = GetPlayerController();
		if (!IsValidObjectPtr(pPlayerController))
			return false;

		return pPlayerController->ProjectWorldLocationToScreen(in, &out, relative);
	}

	// I made this function so I would have to type less to get the screen position of a world position
	static SDK::FVector2D W2S(SDK::FVector in, bool relative = false)
	{
		SDK::FVector2D out = { 0, 0 };
		SDK::APlayerController* PlayerController = GetPlayerController();
		if (!IsValidObjectPtr(PlayerController))
			return out;

		PlayerController->ProjectWorldLocationToScreen(in, &out, relative);

		return out;
	}

	enum class EActorType{
		Other,
		Guard,
		Shield,
		Dozer,
		Cloaker,
		Sniper,
		Grenadier,
		Taser,
		Techie,
		Civilian,
		ObjectiveItem,
		InteractableItem,
		LootBag,
		
		Max
	};

	struct ActorInfo{
		EActorType m_eType;
		float m_flDistance;
		SDK::AActor* m_pActor;
	};

	EActorType DetermineActorType(SDK::AActor* pActor){
        if (pActor->IsA(SDK::ACH_SWAT_SHIELD_C::StaticClass()))
            return EActorType::Shield;
        else if (pActor->IsA(SDK::ACH_Dozer_C::StaticClass()))
            return EActorType::Dozer;
        else if (pActor->IsA(SDK::ACH_Cloaker_C::StaticClass()))
            return EActorType::Cloaker;
        else if (pActor->IsA(SDK::ACH_Sniper_C::StaticClass()))
            return EActorType::Sniper;
        else if (pActor->IsA(SDK::ACH_Grenadier_C::StaticClass()))
            return EActorType::Grenadier;
        else if (pActor->IsA(SDK::ACH_Taser_C::StaticClass()))
            return EActorType::Taser;
        else if (pActor->IsA(SDK::ACH_Tower_C::StaticClass()))
            return EActorType::Techie;

        if(pActor->IsA(SDK::ACH_BaseCop_C::StaticClass()))
            return EActorType::Guard;

        if(pActor->IsA(SDK::ACH_BaseCivilian_C::StaticClass()))
            return EActorType::Civilian;

        static std::vector<SDK::UClass*> aObjectiveItemClasses = {
            SDK::ABP_RFIDTagBase_C::StaticClass(),
            SDK::ABP_KeycardBase_C::StaticClass(),
            SDK::ABP_CarriedInteractableBase_C::StaticClass(),
            SDK::UGE_CarKeys_C::StaticClass(),
            SDK::UGA_Phone_C::StaticClass()
        };
        if(!std::none_of(aObjectiveItemClasses.begin(), aObjectiveItemClasses.end(), [pActor](SDK::UClass* pClass) { return pActor->IsA(pClass); }))
            return EActorType::ObjectiveItem;
        
        if(pActor->IsA(SDK::ASBZInteractionActor::StaticClass()))
            return EActorType::InteractableItem;

        if(pActor->IsA(SDK::ABP_BaseValuableBag_C::StaticClass()))
            return EActorType::LootBag;

        return EActorType::Other;        
    }

	bool ShouldSkipActor(SDK::AActor* pActor, EActorType eType){
        switch(eType){
        case EActorType::Guard:
        case EActorType::Shield:
        case EActorType::Dozer:
        case EActorType::Cloaker:
        case EActorType::Sniper:
        case EActorType::Grenadier:
        case EActorType::Taser:
        case EActorType::Techie:
        case EActorType::Civilian:
            return !reinterpret_cast<SDK::ASBZCharacter*>(pActor)->bIsAlive;

        default:
            break;
        }
        return false;
    }
};