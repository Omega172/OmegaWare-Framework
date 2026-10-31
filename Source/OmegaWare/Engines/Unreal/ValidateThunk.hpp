#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

namespace OmegaWare::Unreal
{
	inline constexpr std::size_t kValidateThunkScan = 0x300;

	[[nodiscard]] std::optional<std::size_t> FindValidateCall(const std::uint8_t* pThunk, std::size_t uSize, std::wstring_view strExpected);
}
