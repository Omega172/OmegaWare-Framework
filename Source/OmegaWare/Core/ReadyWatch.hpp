#pragma once

#include <chrono>
#include <cstdint>
#include <string_view>

namespace OmegaWare::ReadyWatch
{
	enum class EResult : std::uint8_t
	{
		Rendering,
		Fallback,
		TimedOut,
		Cancelled,
	};

	void Start();
	[[nodiscard]] EResult Wait(std::chrono::milliseconds msTimeout);
	void Stop();
	[[nodiscard]] std::string_view ApiName();
	[[nodiscard]] int Presents();
}
