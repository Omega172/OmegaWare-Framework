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
};