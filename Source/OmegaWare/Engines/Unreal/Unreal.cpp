#include "Engines/Unreal/Unreal.hpp"

#if OMEGAWARE_ENGINE_UNREAL

#include "Core/Framework.hpp"
#include "Core/Log.hpp"
#include "Engines/Unreal/ProcessEvent.hpp"
#include "Engines/Unreal/Resolver.hpp"

#pragma warning(push)
#pragma warning(disable : 4005 4369 4309 4267 4244)
#include "SDK/Engine_parameters.hpp"
#pragma warning(pop)
#include "Memory/Safety.hpp"

#include <thread>

namespace OmegaWare::Unreal
{
	namespace
	{
		template <typename T>
		[[nodiscard]] T* Checked(T* pObject)
		{
			return Memory::IsValidObjectPtr(pObject) ? pObject : nullptr;
		}
	}

	bool Initialize()
	{
		return Resolver::Resolve();
	}

	bool IsResolved()
	{
		return Resolver::IsResolved();
	}

	bool WaitForWorld(std::chrono::milliseconds msTimeout)
	{
		const auto tpDeadline = std::chrono::steady_clock::now() + msTimeout;
		bool bLogged = false;
		while (!GetWorld())
		{
			if (IsUnloadRequested() || std::chrono::steady_clock::now() >= tpDeadline)
			{
				LogWarning("GWorld did not become available");
				return false;
			}
			if (!bLogged)
			{
				LogInfo("waiting for GWorld");
				bLogged = true;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
		LogInfo("GWorld ready");
		return true;
	}

	SDK::UWorld* GetWorld()
	{
		return Resolver::World();
	}

	SDK::AGameStateBase* GetGameState()
	{
		SDK::UWorld* pWorld = GetWorld();
		return pWorld ? Checked(OW_FIELD(pWorld, GameState)) : nullptr;
	}

	SDK::UGameInstance* GetGameInstance()
	{
		SDK::UWorld* pWorld = GetWorld();
		return pWorld ? Checked(OW_FIELD(pWorld, OwningGameInstance)) : nullptr;
	}

	SDK::ULocalPlayer* GetLocalPlayer(int iIndex)
	{
		SDK::UGameInstance* pGameInstance = GetGameInstance();
		if (!pGameInstance)
			return nullptr;
		auto& rLocalPlayers = OW_FIELD(pGameInstance, LocalPlayers);
		if (!Memory::IsValidObjectPtr(&rLocalPlayers) || !rLocalPlayers.IsValidIndex(iIndex))
			return nullptr;
		return Checked(rLocalPlayers[iIndex]);
	}

	SDK::UGameViewportClient* GetViewportClient()
	{
		SDK::ULocalPlayer* pLocalPlayer = GetLocalPlayer();
		return pLocalPlayer ? Checked(OW_FIELD(pLocalPlayer, ViewportClient)) : nullptr;
	}

	SDK::APlayerController* GetPlayerController()
	{
		SDK::ULocalPlayer* pLocalPlayer = GetLocalPlayer();
		return pLocalPlayer ? Checked(OW_FIELD(pLocalPlayer, PlayerController)) : nullptr;
	}

	SDK::APawn* GetLocalPawn()
	{
		SDK::APlayerController* pController = GetPlayerController();
		return pController ? Checked(OW_FIELD(pController, AcknowledgedPawn)) : nullptr;
	}

	SDK::APlayerCameraManager* GetCameraManager()
	{
		SDK::APlayerController* pController = GetPlayerController();
		return pController ? Checked(OW_FIELD(pController, PlayerCameraManager)) : nullptr;
	}

	SDK::UKismetMathLibrary* MathLibrary()
	{
		return SDK::UKismetMathLibrary::GetDefaultObj();
	}

	SDK::UKismetSystemLibrary* SystemLibrary()
	{
		return SDK::UKismetSystemLibrary::GetDefaultObj();
	}

	SDK::UGameplayStatics* GameplayStatics()
	{
		return SDK::UGameplayStatics::GetDefaultObj();
	}

	SDK::UKismetStringLibrary* StringLibrary()
	{
		return SDK::UKismetStringLibrary::GetDefaultObj();
	}

	std::optional<SDK::FVector2D> WorldToScreen(const SDK::FVector& vecWorld, bool bRelativeToViewport)
	{
		SDK::APlayerController* pController = GetPlayerController();
		if (!pController)
			return std::nullopt;
		SDK::FVector2D vecScreen{};
		if (!pController->ProjectWorldLocationToScreen(vecWorld, &vecScreen, bRelativeToViewport))
			return std::nullopt;
		return vecScreen;
	}

	bool LogDiagnostics()
	{
		if (!IsResolved())
		{
			LogError("unreal checks: the game's globals were not resolved");
			return false;
		}
		Resolver::LogReport();
		SDK::UWorld* pWorld = GetWorld();
		LogInfo("unreal check: world {} game instance {} local player {}", static_cast<void*>(pWorld), static_cast<void*>(GetGameInstance()), static_cast<void*>(GetLocalPlayer()));
		LogInfo("unreal check: controller {} pawn {} camera manager {}", static_cast<void*>(GetPlayerController()), static_cast<void*>(GetLocalPawn()), static_cast<void*>(GetCameraManager()));
		const int iSum = SDK::UKismetMathLibrary::Add_IntInt(2, 3);
		LogInfo("unreal check: KismetMathLibrary::Add_IntInt(2, 3) = {} {}", iSum, iSum == 5 ? "ok" : "FAILED");

		constexpr std::string_view strAdd = "Engine.KismetMathLibrary.Add_IntInt";
		int iSeen = 0;
		const HookHandle uChange = OnFunction<SDK::Params::KismetMathLibrary_Add_IntInt>(strAdd,
			[](SDK::UObject*, SDK::Params::KismetMathLibrary_Add_IntInt& rParams, bool&) { rParams.A += 10; },
			[&iSeen](SDK::UObject*, SDK::Params::KismetMathLibrary_Add_IntInt& rParams) { iSeen = rParams.ReturnValue; });
		const int iChanged = SDK::UKismetMathLibrary::Add_IntInt(2, 3);
		RemoveProcessEvent(uChange);
		const bool bChanged = uChange != kInvalidHook && iChanged == 15 && iSeen == 15;
		LogInfo("unreal check: process event hook changed a parameter (got {}, after callback saw {}) {}", iChanged, iSeen, bChanged ? "ok" : "FAILED");

		const HookHandle uSkip = OnFunction<SDK::Params::KismetMathLibrary_Add_IntInt>(strAdd,
			[](SDK::UObject*, SDK::Params::KismetMathLibrary_Add_IntInt& rParams, bool& rbSkip) { rParams.ReturnValue = 99; rbSkip = true; });
		const int iSkipped = SDK::UKismetMathLibrary::Add_IntInt(2, 3);
		RemoveProcessEvent(uSkip);
		const int iRestored = SDK::UKismetMathLibrary::Add_IntInt(2, 3);
		const bool bSkipped = uSkip != kInvalidHook && iSkipped == 99 && iRestored == 5;
		LogInfo("unreal check: process event hook skipped the original (got {}, {} after removal) {}", iSkipped, iRestored, bSkipped ? "ok" : "FAILED");

		const bool bPassed = pWorld && iSum == 5 && bChanged && bSkipped;
		LogInfo("unreal checks {}", bPassed ? "passed" : "FAILED");
		return bPassed;
	}
}

#endif
