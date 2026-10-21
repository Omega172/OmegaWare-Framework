#pragma once

#if OMEGAWARE_ENGINE_UNREAL

#pragma warning(push)
#pragma warning(disable : 4005 4369 4309 4267 4244)
#include "SDK/Engine_classes.hpp"
#pragma warning(pop)

#include "Engines/Unreal/Field.hpp"

#include <chrono>
#include <optional>

namespace OmegaWare::Unreal
{
	bool Initialize();
	[[nodiscard]] bool IsResolved();
	bool WaitForWorld(std::chrono::milliseconds msTimeout);

	[[nodiscard]] SDK::UWorld* GetWorld();
	[[nodiscard]] SDK::AGameStateBase* GetGameState();
	[[nodiscard]] SDK::UGameInstance* GetGameInstance();
	[[nodiscard]] SDK::ULocalPlayer* GetLocalPlayer(int iIndex = 0);
	[[nodiscard]] SDK::UGameViewportClient* GetViewportClient();
	[[nodiscard]] SDK::APlayerController* GetPlayerController();
	[[nodiscard]] SDK::APawn* GetLocalPawn();
	[[nodiscard]] SDK::APlayerCameraManager* GetCameraManager();

	[[nodiscard]] SDK::UKismetMathLibrary* MathLibrary();
	[[nodiscard]] SDK::UKismetSystemLibrary* SystemLibrary();
	[[nodiscard]] SDK::UGameplayStatics* GameplayStatics();
	[[nodiscard]] SDK::UKismetStringLibrary* StringLibrary();

	[[nodiscard]] std::optional<SDK::FVector2D> WorldToScreen(const SDK::FVector& vecWorld, bool bRelativeToViewport = false);
	bool LogDiagnostics();
}

#endif
