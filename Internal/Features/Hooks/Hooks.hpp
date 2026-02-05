#pragma once
#include "pch.h"

using UObjectProcessEvent_t = void(*)(const SDK::UObject*, class SDK::UFunction*, void*);
inline UObjectProcessEvent_t UObjectProcessEvent_o = nullptr;
inline std::vector<std::function<bool(const SDK::UObject*, class SDK::UFunction*, void*)>> m_vecUObjectProcessEventCallbacks;
inline UObjectProcessEvent_t UObjectProcessEventPlayer_o = nullptr;
inline std::vector<std::function<bool(const SDK::UObject*, class SDK::UFunction*, void*)>> m_vecUObjectProcessEventPlayerCallbacks;

using ULocalPlayerGetViewPoint_t = void(*)(SDK::ULocalPlayer*, SDK::FMinimalViewInfo*);
inline ULocalPlayerGetViewPoint_t ULocalPlayerGetViewPoint_o = nullptr;
inline std::vector<std::function<void(SDK::ULocalPlayer*, SDK::FMinimalViewInfo*)>> m_vecULocalPlayerGetViewPointCallbacks;

using APlayerControllerGetPlayerViewPoint_t = void(*)(SDK::APlayerController*, SDK::FVector*, SDK::FRotator*);
inline APlayerControllerGetPlayerViewPoint_t APlayerControllerGetPlayerViewPoint_o = nullptr;
inline std::vector<std::function<void(SDK::APlayerController*, SDK::FVector*, SDK::FRotator*)>> m_vecAPlayerControllerGetPlayerViewPointCallbacks;

static void UObjectProcessEvent_hk(const SDK::UObject* pObject, class SDK::UFunction* pFunction, void* pParams) {
    if (pObject->IsA(SDK::UKismetStringLibrary::StaticClass())) {
        UObjectProcessEvent_o(pObject, pFunction, pParams);
        return;
    }

    if (m_vecUObjectProcessEventCallbacks.size() > 0) {
        for (const auto& callback : m_vecUObjectProcessEventCallbacks) {
            if (callback(pObject, pFunction, pParams))
                return;
        }
    }

    UObjectProcessEvent_o(pObject, pFunction, pParams);
}

static void UObjectProcessEventPlayer_hk(const SDK::UObject* pObject, class SDK::UFunction* pFunction, void* pParams) {
    if (m_vecUObjectProcessEventPlayerCallbacks.size() > 0) {
        for (const auto& callback : m_vecUObjectProcessEventPlayerCallbacks) {
            if (callback(pObject, pFunction, pParams))
                return;
        }
    }

    UObjectProcessEventPlayer_o(pObject, pFunction, pParams);
}

static void ULocalPlayerGetViewPoint_hk(SDK::ULocalPlayer* _this, SDK::FMinimalViewInfo* OutViewInfo) {
    ULocalPlayerGetViewPoint_o(_this, OutViewInfo);

    if (m_vecULocalPlayerGetViewPointCallbacks.size() > 0) {
        for (const auto& callback : m_vecULocalPlayerGetViewPointCallbacks) {
            callback(_this, OutViewInfo);
        }
    }
}

static void APlayerControllerGetPlayerViewPoint_hk(SDK::APlayerController* _this, SDK::FVector* out_Location, SDK::FRotator* out_Rotation) {
    APlayerControllerGetPlayerViewPoint_o(_this, out_Location, out_Rotation);

    if (m_vecAPlayerControllerGetPlayerViewPointCallbacks.size() > 0) {
        for (const auto& callback : m_vecAPlayerControllerGetPlayerViewPointCallbacks) {
            callback(_this, out_Location, out_Rotation);
        }
    }
}

class Hooks : public BaseFeature
{
public:

	bool Setup();

	void Destroy();

	void Run() {};

	std::string GetName() { return "Hooks"; };
};

std::unique_ptr<Hooks> pHooks = std::make_unique<Hooks>();