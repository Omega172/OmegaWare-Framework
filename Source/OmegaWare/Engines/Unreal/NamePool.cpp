#include "Engines/Unreal/NamePool.hpp"

#include <cstring>
#include <utility>

namespace OmegaWare::Unreal::NamePool
{
	namespace
	{
		[[nodiscard]] std::uint16_t ReadHeader(const std::uint8_t* pEntry)
		{
			std::uint16_t uHeader = 0;
			std::memcpy(&uHeader, pEntry, sizeof(uHeader));
			return uHeader;
		}

		[[nodiscard]] std::size_t EntryLength(std::uint16_t uHeader, const stLayout& stPool)
		{
			return stPool.iLengthShift == 6 ? (uHeader >> 6) & 0x3FF : uHeader >> 1;
		}
	}

	std::optional<stLayout> DetectLayout(const std::uint8_t* pFirstBlock)
	{
		if (!pFirstBlock)
			return std::nullopt;
		for (const auto [uHeaderOffset, uStride] : { std::pair{ std::size_t{ 0 }, kStride }, std::pair{ std::size_t{ 4 }, kCasePreservingStride } })
		{
			const std::size_t uNoneEnd = uHeaderOffset + 2 + 4;
			const std::size_t uSecond = (uNoneEnd + uStride - 1) / uStride * uStride;
			if (std::memcmp(pFirstBlock + uHeaderOffset + 2, "None", 4) != 0 || std::memcmp(pFirstBlock + uSecond + uHeaderOffset + 2, "ByteProperty", 12) != 0)
				continue;
			const std::uint16_t uNone = ReadHeader(pFirstBlock + uHeaderOffset);
			const std::uint16_t uByteProperty = ReadHeader(pFirstBlock + uSecond + uHeaderOffset);
			if ((uNone & 1) || (uByteProperty & 1))
				continue;
			for (const int iShift : { 6, 1 })
			{
				const stLayout stPool{ iShift, uHeaderOffset, uStride };
				if (EntryLength(uNone, stPool) == 4 && EntryLength(uByteProperty, stPool) == 12)
					return stPool;
			}
		}
		return std::nullopt;
	}

	std::size_t Decode(const std::uint8_t* const* ppBlocks, std::uint32_t uBlockCount, std::uint32_t uIndex, const stLayout& stPool, wchar_t* pOut, std::size_t uCapacity)
	{
		const std::uint32_t uBlock = uIndex >> 16;
		const std::size_t uOffset = static_cast<std::size_t>(uIndex & 0xFFFF) * stPool.uStride;
		const std::size_t uBlockBytes = stPool.BlockBytes();
		if (!ppBlocks || !pOut || !uCapacity || uBlock >= uBlockCount || !ppBlocks[uBlock] || uOffset + stPool.uHeaderOffset + 2 > uBlockBytes)
			return 0;
		const std::uint8_t* pEntry = ppBlocks[uBlock] + uOffset + stPool.uHeaderOffset;
		const std::uint16_t uHeader = ReadHeader(pEntry);
		const bool bWide = (uHeader & 1) != 0;
		const std::size_t uLength = EntryLength(uHeader, stPool);
		if (!uLength || uLength > kMaxLength || uLength > uCapacity || uOffset + stPool.uHeaderOffset + 2 + uLength * (bWide ? 2 : 1) > uBlockBytes)
			return 0;
		const std::uint8_t* pCharacters = pEntry + 2;
		for (std::size_t i = 0; i < uLength; ++i)
		{
			if (bWide)
			{
				char16_t chValue = 0;
				std::memcpy(&chValue, pCharacters + i * 2, sizeof(chValue));
				pOut[i] = static_cast<wchar_t>(chValue);
			}
			else
				pOut[i] = static_cast<wchar_t>(pCharacters[i]);
		}
		return uLength;
	}

	bool Equals(const wchar_t* pName, std::size_t uLength, std::string_view strText)
	{
		if (uLength != strText.size())
			return false;
		for (std::size_t i = 0; i < uLength; ++i)
			if (pName[i] != static_cast<wchar_t>(static_cast<unsigned char>(strText[i])))
				return false;
		return true;
	}
}
