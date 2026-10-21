#include "Engines/Engine.hpp"
#include "Core/Framework.hpp"
#include "Core/Log.hpp"
#include "Memory/Memory.hpp"
#if OMEGAWARE_ENGINE_UNITY
#include "Engines/Unity/Il2Cpp.hpp"
#include "Engines/Unity/Mono.hpp"
#endif
#if OMEGAWARE_ENGINE_UNREAL
#include "Engines/Unreal/ProcessEvent.hpp"
#include "Engines/Unreal/Unreal.hpp"
#include "Engines/Unreal/SdkAudit.hpp"
#endif

#include <atomic>
#include <chrono>
#include <thread>

namespace OmegaWare::Engine
{
	namespace
	{
		std::atomic<bool> g_bReady = false;
		constexpr auto kWorldTimeout = std::chrono::seconds(OMEGAWARE_PROXY ? 300 : 60);
		constexpr auto kUnityTimeout = std::chrono::seconds(OMEGAWARE_PROXY ? 120 : 30);

#if OMEGAWARE_ENGINE_UNITY
		bool TryUnityRuntime()
		{
			if (Memory::GetModule("GameAssembly.dll"))
				return Il2Cpp::Initialize();
			for (std::string_view strModule : { "mono-2.0-bdwgc.dll", "mono-2.0-sgen.dll", "mono.dll" })
				if (Memory::GetModule(strModule))
					return Mono::Initialize();
			return false;
		}

		bool WaitForUnity(std::chrono::milliseconds msTimeout)
		{
			const auto tpDeadline = std::chrono::steady_clock::now() + msTimeout;
			while (!TryUnityRuntime())
			{
				if (IsUnloadRequested() || std::chrono::steady_clock::now() >= tpDeadline)
				{
					LogWarning("no usable mono or il2cpp runtime appeared in time");
					return false;
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(500));
			}
			return true;
		}
#endif
	}

	bool Initialize(const std::function<bool()>& fnHostReady)
	{
		LogInfo("engine: {}", ToString(kEngine));
		if (OMEGAWARE_PROXY && kEngine != EEngine::None && !fnHostReady())
		{
			LogInfo("waiting for the game's first frame before touching the engine");
			const auto tpDeadline = std::chrono::steady_clock::now() + kWorldTimeout;
			while (!fnHostReady())
			{
				if (IsUnloadRequested() || std::chrono::steady_clock::now() >= tpDeadline)
				{
					LogWarning("the game never presented a frame, engine {} was not started", ToString(kEngine));
					return false;
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			}
			LogInfo("first frame presented, starting engine {}", ToString(kEngine));
		}
#if OMEGAWARE_ENGINE_UNREAL
		if (!Unreal::Initialize())
			LogError("the Unreal SDK could not be matched to this game, engine unreal was not started");
		else
			g_bReady = Unreal::WaitForWorld(kWorldTimeout);
		if (g_bReady)
		{
			Unreal::LogDiagnostics();
			Unreal::SdkAudit::Run();
		}
#elif OMEGAWARE_ENGINE_UNITY
		g_bReady = WaitForUnity(kUnityTimeout);
		if (Mono::IsAvailable())
			Mono::LogDiagnostics();
		else if (Il2Cpp::IsAvailable())
			Il2Cpp::LogDiagnostics();
#else
		g_bReady = true;
#endif
		if (!g_bReady)
			LogWarning("engine {} is not ready, engine-dependent features may not work", ToString(kEngine));
		return g_bReady;
	}

	void Update()
	{
#if OMEGAWARE_ENGINE_UNREAL
		if (g_bReady)
			Unreal::RetryPendingProcessEvents();
#endif
	}

	void Shutdown()
	{
#if OMEGAWARE_ENGINE_UNREAL
		Unreal::ShutdownProcessEvent();
#endif
#if OMEGAWARE_ENGINE_UNITY
		Mono::Shutdown();
		Il2Cpp::Shutdown();
#endif
		g_bReady = false;
	}

	bool IsReady()
	{
		return g_bReady;
	}
}
