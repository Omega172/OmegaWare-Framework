#include "pch.h"
#include "Hooks.hpp"

bool Hooks::Setup() {
    SDK::UWorld* pGWorld = SDK::UWorld::GetWorld();
	if (!pGWorld)
		return{};

    return true;
}

void Hooks::Destroy() {

}

void Hooks::Run() {

}