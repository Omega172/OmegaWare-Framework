#include "Engines/Unreal/NamePool.hpp"

#include <cstring>

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
		if (!pFirstBlock || std::memcmp(pFirstBlock + 2, "None", 4) != 0 || std::memcmp(pFirstBlock + 8, "ByteProperty", 12) != 0)
			return std::nullopt;
		const std::uint16_t uNone = ReadHeader(pFirstBlock);
		const std::uint16_t uByteProperty = ReadHeader(pFirstBlock + 6);
		if ((uNone & 1) || (uByteProperty & 1))
			return std::nullopt;
		for (const int iShift : { 6, 1 })
		{
			const stLayout stPool{ iShift };
			if (EntryLength(uNone, stPool) == 4 && EntryLength(uByteProperty, stPool) == 12)
				return stPool;
		}
		return std::nullopt;
	}

	std::size_t Decode(const std::uint8_t* const* ppBlocks, std::uint32_t uBlockCount, std::uint32_t uIndex, const stLayout& stPool, wchar_t* pOut, std::size_t uCapacity)
	{
		const std::uint32_t uBlock = uIndex >> 16;
		const std::size_t uOffset = static_cast<std::size_t>(uIndex & 0xFFFF) * kStride;
		if (!ppBlocks || !pOut || !uCapacity || uBlock >= uBlockCount || !ppBlocks[uBlock] || uOffset + 2 > kBlockBytes)
			return 0;
		const std::uint8_t* pEntry = ppBlocks[uBlock] + uOffset;
		const std::uint16_t uHeader = ReadHeader(pEntry);
		const bool bWide = (uHeader & 1) != 0;
		const std::size_t uLength = EntryLength(uHeader, stPool);
		if (!uLength || uLength > kMaxLength || uLength > uCapacity || uOffset + 2 + uLength * (bWide ? 2 : 1) > kBlockBytes)
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
