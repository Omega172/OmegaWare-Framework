#include "Engines/Unreal/ValidateThunk.hpp"

#include "Memory/Safety.hpp"

#include <cstring>
#include <cwchar>

namespace OmegaWare::Unreal
{
	namespace
	{
		template <typename T>
		[[nodiscard]] T ReadAt(const std::uint8_t* pCode)
		{
			T tValue{};
			std::memcpy(&tValue, pCode, sizeof(T));
			return tValue;
		}

		[[nodiscard]] bool IsFailureString(const std::uint8_t* pLea, std::wstring_view strExpected)
		{
			if (!Memory::IsReadable(pLea, 7) || pLea[0] != 0x48 || pLea[1] != 0x8D || pLea[2] != 0x0D)
				return false;
			const auto* pString = reinterpret_cast<const wchar_t*>(pLea + 7 + ReadAt<std::int32_t>(pLea + 3));
			if (Memory::SafeWcslen(pString, strExpected.size() + 1) != strExpected.size())
				return false;
			return std::wmemcmp(pString, strExpected.data(), strExpected.size()) == 0;
		}
	}

	std::optional<std::size_t> FindValidateCall(const std::uint8_t* pThunk, std::size_t uSize, std::wstring_view strExpected)
	{
		constexpr std::size_t kLongest = 6 + 2 + 6;
		if (!pThunk || strExpected.empty() || uSize < kLongest || !Memory::IsReadable(pThunk, uSize))
			return std::nullopt;

		std::optional<std::size_t> optFound;
		for (std::size_t uPos = 0; uPos + kLongest <= uSize; ++uPos)
		{
			const std::uint8_t* pCall = pThunk + uPos;
			if (pCall[0] != 0xFF)
				continue;
			const std::uint8_t uModRm = pCall[1];
			std::size_t uOffset = 0;
			std::size_t uLength = 0;
			if ((uModRm & 0xF8) == 0x90 && uModRm != 0x94)
			{
				uOffset = ReadAt<std::uint32_t>(pCall + 2);
				uLength = 6;
			}
			else if ((uModRm & 0xF8) == 0x50 && uModRm != 0x54 && pCall[2] < 0x80)
			{
				uOffset = pCall[2];
				uLength = 3;
			}
			else
				continue;

			const std::uint8_t* pTest = pCall + uLength;
			if (pTest[0] != 0x84 || pTest[1] != 0xC0 || uOffset % sizeof(void*) != 0)
				continue;
			const std::uint8_t* pBranch = pTest + 2;
			const std::uint8_t* pFailure = nullptr;
			if (pBranch[0] == 0x75)
				pFailure = pBranch + 2;
			else if (pBranch[0] == 0x0F && pBranch[1] == 0x85)
				pFailure = pBranch + 6;
			else if (pBranch[0] == 0x74)
				pFailure = pBranch + 2 + static_cast<std::int8_t>(pBranch[1]);
			else if (pBranch[0] == 0x0F && pBranch[1] == 0x84)
				pFailure = pBranch + 6 + ReadAt<std::int32_t>(pBranch + 2);
			if (!pFailure || !IsFailureString(pFailure, strExpected))
				continue;
			if (optFound && *optFound != uOffset)
				return std::nullopt;
			optFound = uOffset;
		}
		return optFound;
	}
}
