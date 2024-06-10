#pragma once
#include "pch.h"

#ifdef DUMPER_7
#include "BasicTypes.h"
#endif

#if FRAMEWORK_UNREAL // Not sure if this is needed but it's here anyway

inline bool FrameworkUnrealInit()
{
#ifndef DUMPER_7
	if (!CG::InitSdk())
		return false;

	if (!(*CG::UWorld::GWorld))
		LogErrorHere("Waiting for GWorld to initalize");

	while (!(*CG::UWorld::GWorld))
		continue;
#endif // !DUMPER_7

#ifdef DUMPER_7
	CG::UWorld* pGWorld = CG::UWorld::GetWorld();
	if (!pGWorld)
		LogErrorHere("Waiting for GWorld to initalize");

	while (!pGWorld)
		continue;
#endif // DUMPER_7

	return true;
}

#ifndef DUMPER_7
typedef void(__thiscall* PostRender) (CG::UObject* pViewportClient, CG::UCanvas* pCanvas);
class BadHookPostRender
{
public:
	static CG::UFont* pFont;
	static DWORD dwOldProtect;

	static PostRender oPostRender;

	static void HookPostRender()
	{
		pFont = CG::UObject::FindObject<CG::UFont>("Font Roboto.Roboto");
		if (!IsValidObjectPtr(pFont))
			return;

		CG::UGameViewportClient* pViewportClient = Framework::unreal.get()->GetViewportClient();;
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
		CG::UGameViewportClient* pViewportClient = Framework::unreal.get()->GetViewportClient();
		if (!IsValidObjectPtr(pViewportClient))
			return;

		void** VFTable = pViewportClient->VfTable;
		VirtualProtect(&VFTable[POST_RENDER_INDEX], 8, PAGE_EXECUTE_READWRITE, &dwOldProtect);
		VFTable[POST_RENDER_INDEX] = oPostRender;
		VirtualProtect(&VFTable[POST_RENDER_INDEX], 8, dwOldProtect, &dwOldProtect);
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

#define CREATE_ENUM(n) n,
#define CREATE_CLASS_LOOKUP(n) FNames::ClassLookupEntry_t{std::string_view(#n), 0, FNames::EFNames::n},
namespace FNames
{
	enum EFNames {
		GAME_FNAMES(CREATE_ENUM)
	};

	typedef struct ActorInfo_t {
		CG::AActor* pActor;
		EFNames iLookupIndex;
		float flDistance;
	} PawnInfo_t;

	typedef struct ClassLookupEntry_t {
		std::string_view sName;
		int32_t ComparisonIndex;
		EFNames iLookupIndex;
	} ClassLookupEntry_t;

	inline std::vector<ClassLookupEntry_t> vecClassLookups{
		GAME_FNAMES(CREATE_CLASS_LOOKUP)
	};

	inline EFNames GetLookupIndex(int32_t ComparisonIndex) {
		for (const auto entry : vecClassLookups) {
			if (ComparisonIndex == entry.ComparisonIndex)
				return entry.iLookupIndex;
		}

		return EFNames::Invalid;
	}

	inline void Initialize()
	{
#ifdef DUMPER_7
		CG::FNamePool* GNames = reinterpret_cast<CG::FNamePool*>(CG::InSDKUtils::GetImageBase() + CG::Offsets::GNames);
#else
		CG::FNamePool* GNames = CG::FName::GNames;
#endif

		LogDebugStreamHere("GNames: 0x" << GNames);
		LogDebugStreamHere("GNames Count: " << GNames->Count());

		size_t iGNameSize = 0;
		int lastBlock = 0;
		uintptr_t nextFNameAddress = reinterpret_cast<uintptr_t>(GNames->Allocator.Blocks[0]);

		while (1) {

		RePlay:
			int32_t nextFNameComparisonId = MAKELONG((uint16_t)((nextFNameAddress - reinterpret_cast<uintptr_t>(GNames->Allocator.Blocks[lastBlock])) / 2), (uint16_t)lastBlock);
			int32_t block = nextFNameComparisonId >> 16;
			int32_t offset = (uint16_t)nextFNameComparisonId;
			int32_t offsetFromBlock = static_cast<int32_t>(nextFNameAddress - reinterpret_cast<uintptr_t>(GNames->Allocator.Blocks[lastBlock]));

			// Get entry information
			const uintptr_t entryOffset = nextFNameAddress;
			const int toAdd = 0x00 + 0x02; // HeaderOffset + HeaderSize

			const uint16_t* pNameHeader = reinterpret_cast<uint16_t*>(entryOffset);

			if (!IsValidObjectPtr(pNameHeader))
				break;

			const uint16_t nameHeader = *pNameHeader;
			int nameLength = nameHeader >> 6;
			bool isWide = (nameHeader & 1) != 0;
			if (isWide)
				nameLength += nameLength;

			// if odd number (odd numbers are aligned with 0x00)
			if (!isWide && nameLength % 2 != 0)
				nameLength += 1;

			// Block end ?
			if (offsetFromBlock + toAdd + (nameLength * 2) >= 0xFFFF * CG::FNameEntryAllocator::Stride || nameHeader == 0x00 || block == GNames->Allocator.CurrentBlock && offset >= GNames->Allocator.CurrentByteCursor)
			{
				nextFNameAddress = reinterpret_cast<uintptr_t>(GNames->Allocator.Blocks[++lastBlock]);
				goto RePlay;
			}

			std::string sName = std::string(reinterpret_cast<CG::FNameEntry*>(entryOffset)->AnsiName, nameHeader >> 6);

			for (int i = 0; i < vecClassLookups.size(); i++) {
				if (vecClassLookups[i].ComparisonIndex)
					continue;

				if (sName != vecClassLookups[i].sName)
					continue;

				vecClassLookups[i].ComparisonIndex = nextFNameComparisonId;
				Utils::LogDebug(Utils::GetLocation(CurrentLoc), "Initalized " + sName);
				break;
			}

			// We hit last Name in last Block
			if (lastBlock > GNames->Allocator.CurrentBlock)
				break;

			// Get next name address
			nextFNameAddress = entryOffset + toAdd + nameLength;
			++iGNameSize;
		}

		for (ClassLookupEntry_t stLookupEntry : vecClassLookups) {
			if (!stLookupEntry.ComparisonIndex)
				Utils::LogError(Utils::GetLocation(CurrentLoc), "Didnt Find " + std::string(stLookupEntry.sName));
		}

		LogDebugStreamHere("Resolved GNames Count: " << iGNameSize);
	};
}
#undef CREATE_ENUM
#undef CREATE_CLASS_LOOKUP

class Unreal
{
public:
	std::vector<CG::AActor*> Actors;
	std::vector<FNames::ActorInfo_t> ActorList;
	std::mutex ActorLock;

	// Shortcut functions to get pointers to important classes used for many things
	static CG::UKismetMathLibrary* GetMathLibrary() { return reinterpret_cast<CG::UKismetMathLibrary*>(CG::UKismetMathLibrary::StaticClass()); }
	static CG::UKismetSystemLibrary* GetSystemLibrary() { return reinterpret_cast<CG::UKismetSystemLibrary*>(CG::UKismetSystemLibrary::StaticClass()); }
	static CG::UGameplayStatics* GetGameplayStatics() { return reinterpret_cast<CG::UGameplayStatics*>(CG::UGameplayStatics::StaticClass()); }
	static CG::UKismetStringLibrary* GetStringLibrary() { return reinterpret_cast<CG::UKismetStringLibrary*>(CG::UKismetStringLibrary::StaticClass()); }

	inline void RefreshActorList() // A function to refresh the actor list
	{
		std::vector<CG::AActor*> lActors{};
		std::vector<FNames::ActorInfo_t> lActorList{};
		std::vector<float> AllDistances{};

#ifndef DUMPER_7
		if (CG::UWorld::GWorld == nullptr) {
			ActorLock.lock();
			Actors.clear();
			ActorList.clear();
			ActorLock.unlock();
			return;
		}
#else
		CG::UWorld* pGWorld = CG::UWorld::GetWorld();
		if (pGWorld == nullptr) {
			ActorLock.lock();
			Actors.clear();
			ActorList.clear();
			ActorLock.unlock();
			return;
		}
#endif

		CG::AGameState* pGameState = reinterpret_cast<CG::AGameState*>(GetGameStateBase());
		if (!IsValidObjectPtr(pGameState)) {
			ActorLock.lock();
			Actors.clear();
			ActorList.clear();
			ActorLock.unlock();
			return;
		}

		FNames::EFNames iMatchState = FNames::GetLookupIndex(pGameState->MatchState.ComparisonIndex);
		if (iMatchState != FNames::InProgress) {
			ActorLock.lock();
			Actors.clear();
			ActorList.clear();
			ActorLock.unlock();
			return;
		}

		CG::APawn* pAcknowledgedPawn = GetAcknowledgedPawn();
		if (!pAcknowledgedPawn) {
			ActorLock.lock();
			Actors.clear();
			ActorList.clear();
			ActorLock.unlock();
			return;
		}


		CG::FVector vecLocation = pAcknowledgedPawn->K2_GetActorLocation();


#ifndef DUMPER_7
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

				bool bFailed = false;
				for (CG::AActor* pOtherActor : lActors) {
					if (pOtherActor != Actor)
						continue;

					bFailed = true;
					break;
				}

				if (!bFailed)
					lActors.push_back(Actor);
			}
		}
#else
		for (int i = 0; i < pGWorld->Levels.Num(); i++)
		{
			CG::ULevel* Level = pGWorld->Levels[i];

			if (!Level)
				continue;

			if (!Level->Actors.IsValid() || !Level->Actors.Num())
				continue;

			for (int j = 0; j < Level->Actors.Num(); j++)
			{
				CG::AActor* Actor = Level->Actors[j];
				if (!Actor)
					continue;

				bool bFailed = false;
				for (CG::AActor* pOtherActor : lActors) {
					if (pOtherActor != Actor)
						continue;

					bFailed = true;
					break;
				}

				if (!bFailed)
					lActors.push_back(Actor);
			}
		}
#endif

		for (CG::AActor* pActor : lActors)
		{
			if (!IsValidObjectPtr(pActor))
				continue;

			int32_t iComparisonIndex = pActor->Name.ComparisonIndex;
			FNames::ActorInfo_t stActorInfo{
				pActor,
				FNames::Invalid,
#ifndef DUMPER_7
				vecLocation.Distance(pActor->K2_GetActorLocation())
#else
				vecLocation.GetDistanceTo(pActor->K2_GetActorLocation())
#endif
			};

			for (FNames::ClassLookupEntry_t stEntry : FNames::vecClassLookups) {
				if (iComparisonIndex == stEntry.ComparisonIndex) {
					stActorInfo.iLookupIndex = stEntry.iLookupIndex;
					break;
				}
			}

			if (stActorInfo.iLookupIndex == FNames::Invalid)
				continue;

			while (1) {
				bool bFixed = true;
				for (float flOtherDistance : AllDistances)
				{
					if (flOtherDistance == stActorInfo.flDistance) {
						stActorInfo.flDistance = nextafterf(stActorInfo.flDistance, INFINITY);
						bFixed = false;
						break;
					}
				}

				if (bFixed)
					break;
			}

			lActorList.push_back(stActorInfo);
			AllDistances.push_back(stActorInfo.flDistance);
		}

		std::stable_sort(lActorList.begin(), lActorList.end(), [](FNames::ActorInfo_t A, FNames::ActorInfo_t B) {
			return A.flDistance > B.flDistance;
		});


		ActorLock.lock();

		Actors.clear();
		for (CG::AActor* v : lActors)
			Actors.push_back(v);

		ActorList.clear();
		for (auto v : lActorList)
			ActorList.push_back(v);

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
#ifndef DUMPER_7
		if (!(*CG::UWorld::GWorld))
			return nullptr;

		CG::AGameStateBase* pGameState = (*CG::UWorld::GWorld)->GameState;
		if (!IsValidObjectPtr(pGameState))
			return nullptr;
#else
		CG::UWorld* pGWorld = CG::UWorld::GetWorld();
		if (!pGWorld)
			return nullptr;

		CG::AGameStateBase* pGameState = pGWorld->GameState;
		if (!IsValidObjectPtr(pGameState))
			return nullptr;
#endif
		return pGameState;
	}
	static CG::UGameInstance* GetGameInstance()
	{
#ifndef DUMPER_7
		if (!(*CG::UWorld::GWorld))
			return nullptr;

		CG::UGameInstance* pGameInstance = (*CG::UWorld::GWorld)->OwningGameInstance;
		if (!IsValidObjectPtr(pGameInstance))
			return nullptr;
#else
		CG::UWorld* pGWorld = CG::UWorld::GetWorld();
		if (!pGWorld)
			return nullptr;

		CG::UGameInstance* pGameInstance = pGWorld->OwningGameInstance;
		if (!IsValidObjectPtr(pGameInstance))
			return nullptr;
#endif
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

		std::vector<T> SortedActors{};

		typedef struct SortHack_t {
			T pActor;
			float flDistance;
		} SortHack_t;

		std::vector<SortHack_t> ActorAndDistances{};
		std::vector<float> AllDistances{};

		for (T pActor : actors)
		{
			if (!IsValidObjectPtr(pActor))
				continue;

			SortHack_t stActorHack{
				pActor,
				pActor->GetDistanceTo(pAcknowledgedPawn)
			};

			while (1) {
				bool bFixed = true;
				for (float flOtherDistance : AllDistances)
				{
					if (flOtherDistance == stActorHack.flDistance) {
						stActorHack.flDistance = nextafterf(stActorHack.flDistance, INFINITY);
						bFixed = false;
						break;
					}
				}

				if (bFixed)
					break;
			}

			ActorAndDistances.push_back(stActorHack);
			AllDistances.push_back(stActorHack.flDistance);
		}

		std::stable_sort(ActorAndDistances.begin(), ActorAndDistances.end(), [](SortHack_t A, SortHack_t B) {
			return A.flDistance > B.flDistance;
		});

		for (SortHack_t stHack : ActorAndDistances)
			SortedActors.push_back(stHack.pActor);

		return SortedActors;
	}
};
#endif