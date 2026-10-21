#pragma once

#include <cstdint>
#include <functional>
#include <string_view>

namespace OmegaWare::Engine
{
	enum class EEngine : std::uint8_t
	{
		None,
		Unreal,
		Unity,
	};

#if OMEGAWARE_ENGINE_UNREAL
	inline constexpr EEngine kEngine = EEngine::Unreal;
#elif OMEGAWARE_ENGINE_UNITY
	inline constexpr EEngine kEngine = EEngine::Unity;
#else
	inline constexpr EEngine kEngine = EEngine::None;
#endif

	[[nodiscard]] constexpr std::string_view ToString(EEngine eEngine)
	{
		switch (eEngine)
		{
		case EEngine::Unreal: return "unreal";
		case EEngine::Unity: return "unity";
		default: return "none";
		}
	}

	bool Initialize(const std::function<bool()>& fnHostReady);
	void Update();
	void Shutdown();
	[[nodiscard]] bool IsReady();
}
