#pragma once
#include "pch.h"
#if FRAMEWORK_UNREAL // Not sure if this is needed but it's here anyway

static CG::UFont* pFont;
static DWORD dwOldProtect;

typedef void(__thiscall* PostRender) (CG::UObject* pViewportClient, CG::UCanvas* pCanvas);
static PostRender oPostRender;

class Unreal
{
public:
	static void HookPostRender()
	{
		pFont = CG::UObject::FindObject<CG::UFont>("Font Roboto.Roboto");
		if (!IsValidObjectPtr(pFont))
			return;

		CG::UGameViewportClient* pViewportClient = GetViewportClient();
		if (!IsValidObjectPtr(pViewportClient))
			return;

		void** VFTable = pViewportClient->VfTable;
		VirtualProtect(&VFTable[POST_RENDER_INDEX], 8, PAGE_EXECUTE_READWRITE, &dwOldProtect);
		oPostRender = reinterpret_cast<PostRender>(VFTable[POST_RENDER_INDEX]);
		VFTable[POST_RENDER_INDEX] = &hkPostRender;
		VirtualProtect(&VFTable[POST_RENDER_INDEX], 8, dwOldProtect, &dwOldProtect);
	}

	static void RestorePostRender()
	{
		CG::UGameViewportClient* pViewportClient = GetViewportClient();
		if (!IsValidObjectPtr(pViewportClient))
			return;

		void** VFTable = pViewportClient->VfTable;
		VirtualProtect(&VFTable[POST_RENDER_INDEX], 8, PAGE_EXECUTE_READWRITE, &dwOldProtect);
		VFTable[POST_RENDER_INDEX] = oPostRender;
		VirtualProtect(&VFTable[POST_RENDER_INDEX], 8, dwOldProtect, &dwOldProtect);
	}

	std::vector<CG::AActor*> Actors;
	std::mutex ActorLock;

	// Shortcut functions to get pointers to important classes used for many things
	static CG::UKismetMathLibrary* GetMathLibrary() { return reinterpret_cast<CG::UKismetMathLibrary*>(CG::UKismetMathLibrary::StaticClass()); }
	static CG::UKismetSystemLibrary* GetSystemLibrary() { return reinterpret_cast<CG::UKismetSystemLibrary*>(CG::UKismetSystemLibrary::StaticClass()); }
	static CG::UGameplayStatics* GetGameplayStatics() { return reinterpret_cast<CG::UGameplayStatics*>(CG::UGameplayStatics::StaticClass()); }
	static CG::UKismetStringLibrary* GetStringLibrary() { return reinterpret_cast<CG::UKismetStringLibrary*>(CG::UKismetStringLibrary::StaticClass()); }

	inline void RefreshActorList() // A function to refresh the actor list
	{
		if (CG::UWorld::GWorld == nullptr)
			return;

		if (!(*CG::UWorld::GWorld))
			return;

		ActorLock.lock();
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
		ActorLock.unlock();
	}

	template <typename T>
	inline std::vector<T*> GetActors() // A function to get all the actors of a certain type can be useful for things like ESP where you only want to draw for certain actor types
	{
		std::vector<T*> actors;

		CG::AGameStateBase* pGameState = GetGameStateBase();
		CG::UClass* pComparisonClass = T::StaticClass();
		if (!IsValidObjectPtr(pComparisonClass) || !pGameState || pGameState->ReplicatedWorldTimeSeconds <= 5.f)
			return actors;

		for (CG::AActor* pActor : Actors)
		{
			if (!IsValidObjectPtr(reinterpret_cast<T*>(pActor)))
				continue;

			if (pActor->IsA(pComparisonClass)) // Check if the actor is of the type we want
				actors.push_back(reinterpret_cast<T*>(pActor)); // If it is add it to the vector
		}

		return actors; // Return the vector
	}

	// These functions are to make getting pointers to important classes and objects easier and cleaner
	static CG::AGameStateBase* GetGameStateBase()
	{
		if (!(*CG::UWorld::GWorld))
			return nullptr;

		CG::AGameStateBase* pGameState = (*CG::UWorld::GWorld)->GameState;
		if (!IsValidObjectPtr(pGameState))
			return nullptr;

		return pGameState;
	}
	static CG::UGameInstance* GetGameInstance()
	{
		if (!(*CG::UWorld::GWorld))
			return nullptr;

		CG::UGameInstance* pGameInstance = (*CG::UWorld::GWorld)->OwningGameInstance;
		if (!IsValidObjectPtr(pGameInstance))
			return nullptr;

		return pGameInstance;
	}

	static CG::ULocalPlayer* GetLocalPlayer(int index = 0)
	{
		CG::UGameInstance* pGameInstance = GetGameInstance();
		if (!IsValidObjectPtr(pGameInstance))
			return nullptr;

		CG::ULocalPlayer* pLocalPlayer = pGameInstance->LocalPlayers[index];
		if (!IsValidObjectPtr(pLocalPlayer))
			return nullptr;

		return pLocalPlayer;
	}
	static CG::UGameViewportClient* GetViewportClient()
	{
		CG::ULocalPlayer* pLocalPlayer = GetLocalPlayer();
		if (!IsValidObjectPtr(pLocalPlayer))
			return nullptr;

		CG::UGameViewportClient* pViewportClient = pLocalPlayer->ViewportClient;
		if (!IsValidObjectPtr(pViewportClient))
			return nullptr;

		return pViewportClient;
	}
	static CG::APlayerController* GetPlayerController()
	{
		CG::ULocalPlayer* LocalPlayer = GetLocalPlayer();
		if (!IsValidObjectPtr(LocalPlayer))
			return nullptr;

		CG::APlayerController* pPlayerController = LocalPlayer->PlayerController;
		if (!IsValidObjectPtr(pPlayerController))
			return nullptr;

		return pPlayerController;
	}
	static CG::APawn* GetAcknowledgedPawn()
	{
		CG::APlayerController* pPlayerController = GetPlayerController();
		if (!IsValidObjectPtr(pPlayerController))
			return nullptr;

		CG::APawn* pAcknowledgedPawn = pPlayerController->AcknowledgedPawn;
		if (!IsValidObjectPtr(pAcknowledgedPawn))
			return nullptr;

		return pAcknowledgedPawn;
	}

	static CG::APlayerCameraManager* GetPlayerCameraManager()
	{
		CG::APlayerController* pPlayerController = GetPlayerController();
		if (!IsValidObjectPtr(pPlayerController))
			return nullptr;

		CG::APlayerCameraManager* pPlayerCameraManager = pPlayerController->PlayerCameraManager;
		if (!IsValidObjectPtr(pPlayerCameraManager))
			return nullptr;

		return pPlayerCameraManager;
	}

	static bool WorldToScreen(CG::FVector in, CG::FVector2D& out, bool relative = false)
	{
		CG::APlayerController* pPlayerController = GetPlayerController();
		if (!IsValidObjectPtr(pPlayerController))
			return false;

		return pPlayerController->ProjectWorldLocationToScreen(in, &out, relative);
	}

	// I made this function so I would have to type less to get the screen position of a world position
	static CG::FVector2D W2S(CG::FVector in, bool relative = false)
	{
		CG::FVector2D out = { 0, 0 };
		CG::APlayerController* PlayerController = GetPlayerController();
		if (!IsValidObjectPtr(PlayerController))
			return out;

		PlayerController->ProjectWorldLocationToScreen(in, &out, relative);

		return out;
	}

	template <typename T>
	static std::vector<T> SortActorsByDistance(std::vector<T> actors)
	{
		CG::APawn* pAcknowledgedPawn = GetAcknowledgedPawn();
		if (!IsValidObjectPtr(pAcknowledgedPawn))
			return actors;

		std::vector<T> SortedActors = actors;

		// Remove invalid actors
		SortedActors.erase(std::remove_if(SortedActors.begin(), SortedActors.end(), [](T pActor) {
			return !IsValidObjectPtr(pActor);
			}), SortedActors.end());

		std::stable_sort(SortedActors.begin(), SortedActors.end(), [pAcknowledgedPawn](T ActorA, T ActorB) {

			float DistanceA = ActorA->GetDistanceTo(pAcknowledgedPawn);
			float DistanceB = ActorB->GetDistanceTo(pAcknowledgedPawn);
			if (DistanceA == DistanceB)
				return ActorA->Name.ComparisonIndex < ActorB->Name.ComparisonIndex;

			return DistanceA < DistanceB;
			});

		return SortedActors;
	}

	static void hkPostRender(CG::UObject* pViewportClient, CG::UCanvas* pCanvas)
	{
		CG::ULocalPlayer* pLocalPlayer = GetLocalPlayer();
		if (!IsValidObjectPtr(pLocalPlayer))
			return oPostRender(pViewportClient, pCanvas);

		CG::FLinearColor Cyan = { 0.f, 1.f, 1.f, 1.f };
		CG::FLinearColor Black = { 0.f, 0.f, 0.f, 1.f };

		// The canvas reports its size as the current resolution but in my testing it is always 2048, 1280
		CG::FVector2D TextSize = pCanvas->K2_TextSize(pFont, L"OmegaWare.xyz", { 1.f, 1.f });
		//pCanvas->K2_DrawText(pFont, L"OmegaWare.xyz", { 1024.f - (TextSize.X / 2), 0.f }, { 1.f, 1.f }, Cyan, 1.f, Black, { 0.f, 0.f }, false, false, true, Black);

		//pCanvas->K2_DrawLine({ 0.f, 0.f }, { 1024.f, 640.f }, 1.f, Cyan);

		oPostRender(pViewportClient, pCanvas);
	}
};
#endif
