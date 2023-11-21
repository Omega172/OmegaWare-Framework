#pragma once
#include "pch.h"
#if FRAMEWORK_UNREAL

class Unreal
{
public:
	std::vector<CG::AActor*> Actors;

	static CG::UKismetMathLibrary* GetMathLibrary() { return reinterpret_cast<CG::UKismetMathLibrary*>(CG::UKismetMathLibrary::StaticClass()); }
	static CG::UKismetSystemLibrary* GetSystemLibrary() { return reinterpret_cast<CG::UKismetSystemLibrary*>(CG::UKismetSystemLibrary::StaticClass()); }
	static CG::UGameplayStatics* GetGameplayStatics() { return reinterpret_cast<CG::UGameplayStatics*>(CG::UGameplayStatics::StaticClass()); }

	inline void RefreshActorList()
	{
		if (!(*CG::UWorld::GWorld))
			return;
		
		Actors.clear();

		for (int i = 0; i < (**CG::UWorld::GWorld).Levels.Count(); i++)
		{
			CG::ULevel* Level = (**CG::UWorld::GWorld).Levels[i];

			if (!Level)
				continue;

			if (!Level->NearActors.Data() || !Level->NearActors.Count())
				continue;

			for (int j = 0; j < Level->NearActors.Count(); j++)
			{
				CG::AActor* Actor = Level->NearActors[j];
				if (!Actor)
					continue;

				Actors.push_back(Actor);
			}
		}
	}

	template <typename T>
	inline std::vector<T*> GetActors()
	{
		std::vector<T*> actors;

		for (CG::AActor* actor : Actors)
		{
			if (!actor || !actor->bCanBeDamaged)
				continue;

			if (!Utils::IsReadableMemory(actor, sizeof(actor)))
				continue;

			if (actor->IsA(T::StaticClass()))
				actors.push_back(reinterpret_cast<T*>(actor));
		}

		return actors;
	}

	static CG::AGameStateBase* GetGameStateBase()
	{
		if (!(*CG::UWorld::GWorld))
			return nullptr;

		return (*CG::UWorld::GWorld)->GameState;
	}
	static CG::UGameInstance* GetGameInstance()
	{
		if (!(*CG::UWorld::GWorld))
			return nullptr;

		return (*CG::UWorld::GWorld)->OwningGameInstance;
	}

	static CG::ULocalPlayer* GetLocalPlayer(int index)
	{
		CG::UGameInstance* GameInstance = GetGameInstance();
		if (!GameInstance)
			return nullptr;

		return GameInstance->LocalPlayers[index];
	}
	static CG::UGameViewportClient* GetViewportClient()
	{
		CG::ULocalPlayer* LocalPlayer = GetLocalPlayer(0);
		if (!LocalPlayer)
			return nullptr;

		return LocalPlayer->ViewportClient;
	}
	static CG::APlayerController* GetPlayerController()
	{
		CG::ULocalPlayer* LocalPlayer = GetLocalPlayer(0);
		if (!LocalPlayer)
			return nullptr;

		return LocalPlayer->PlayerController;
	}
	static CG::APawn* GetAcknowledgedPawn()
	{
		CG::APlayerController* PlayerController = GetPlayerController();
		if (!PlayerController)
			return nullptr;

		return PlayerController->AcknowledgedPawn;
	}

	static CG::APlayerCameraManager* GetPlayerCameraManager()
	{
		CG::APlayerController* PlayerController = GetPlayerController();
		if (!PlayerController)
			return nullptr;

		return PlayerController->PlayerCameraManager;
	}

	static CG::FVector2D W2S(CG::FVector in, bool relitive = false)
	{
		CG::FVector2D out = { 0, 0 };
		CG::APlayerController* PlayerController = GetPlayerController();
		if (!PlayerController)
			return out;

		PlayerController->ProjectWorldLocationToScreen(in, &out, false);

		return out;
	}
};
#endif