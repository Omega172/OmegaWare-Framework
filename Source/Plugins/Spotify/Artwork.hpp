#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace Spotify::Artwork
{
	struct stImage
	{
		std::vector<std::uint8_t> aPixels;
		int iWidth = 0;
		int iHeight = 0;
		std::uint32_t uPrimary = 0;
	};

	[[nodiscard]] std::optional<stImage> Decode(std::span<const std::uint8_t> aData, int iMaxSize);
	[[nodiscard]] std::uint32_t PrimaryColor(std::span<const std::uint8_t> aPixelsRGBA);
}
