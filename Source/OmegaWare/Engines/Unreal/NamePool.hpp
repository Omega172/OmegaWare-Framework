#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

namespace OmegaWare::Unreal::NamePool
{
	inline constexpr std::size_t kBlocksOffset = 0x10;
	inline constexpr std::size_t kCurrentBlockOffset = 0x8;
	inline constexpr std::uint32_t kMaxBlocks = 8192;
	inline constexpr std::uint32_t kBlockBytes = 0x20000;
	inline constexpr std::uint32_t kMaxBlockBytes = 0x40000;
	inline constexpr std::size_t kStride = 2;
	inline constexpr std::size_t kCasePreservingStride = 4;
	inline constexpr std::size_t kMaxLength = 1024;

	struct stLayout
	{
		int iLengthShift = 6;
		std::size_t uHeaderOffset = 0;
		std::size_t uStride = kStride;

		[[nodiscard]] constexpr std::size_t BlockBytes() const { return uStride << 16; }
		[[nodiscard]] constexpr bool CasePreserving() const { return uHeaderOffset != 0; }
	};

	[[nodiscard]] std::optional<stLayout> DetectLayout(const std::uint8_t* pFirstBlock);
	[[nodiscard]] std::size_t Decode(const std::uint8_t* const* ppBlocks, std::uint32_t uBlockCount, std::uint32_t uIndex, const stLayout& stPool, wchar_t* pOut, std::size_t uCapacity);
	[[nodiscard]] bool Equals(const wchar_t* pName, std::size_t uLength, std::string_view strText);
}
