#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace OmegaWare::Memory
{
	using PatternByte = std::int16_t;
	inline constexpr PatternByte kWildcard = -1;

	namespace Detail
	{
		[[nodiscard]] constexpr int HexValue(char chValue)
		{
			if (chValue >= '0' && chValue <= '9')
				return chValue - '0';
			if (chValue >= 'a' && chValue <= 'f')
				return chValue - 'a' + 10;
			if (chValue >= 'A' && chValue <= 'F')
				return chValue - 'A' + 10;
			return -1;
		}

		template <typename F>
		constexpr bool ForEachToken(std::string_view strPattern, F&& fnToken)
		{
			std::size_t uIndex = 0;
			while (uIndex < strPattern.size())
			{
				while (uIndex < strPattern.size() && strPattern[uIndex] == ' ')
					++uIndex;
				if (uIndex >= strPattern.size())
					break;
				std::size_t uEnd = uIndex;
				while (uEnd < strPattern.size() && strPattern[uEnd] != ' ')
					++uEnd;
				const std::string_view strToken = strPattern.substr(uIndex, uEnd - uIndex);
				uIndex = uEnd;

				if (strToken == "?" || strToken == "??")
				{
					fnToken(kWildcard);
					continue;
				}
				if (strToken.size() != 2 || HexValue(strToken[0]) < 0 || HexValue(strToken[1]) < 0)
					return false;
				fnToken(static_cast<PatternByte>(HexValue(strToken[0]) * 16 + HexValue(strToken[1])));
			}
			return true;
		}

		[[nodiscard]] consteval std::size_t CountTokens(std::string_view strPattern)
		{
			std::size_t uCount = 0;
			if (!ForEachToken(strPattern, [&](PatternByte) { ++uCount; }))
				throw std::invalid_argument("pattern tokens must be two hex digits, ? or ??");
			if (uCount == 0)
				throw std::invalid_argument("pattern is empty");
			return uCount;
		}
	}

	template <std::size_t N>
	struct stPatternString
	{
		char aText[N]{};

		consteval stPatternString(const char (&aSource)[N])
		{
			std::copy_n(aSource, N, aText);
		}

		[[nodiscard]] constexpr std::string_view View() const { return { aText, N - 1 }; }
	};

	template <stPatternString strPattern>
	[[nodiscard]] consteval auto MakePattern()
	{
		std::array<PatternByte, Detail::CountTokens(strPattern.View())> aBytes{};
		std::size_t uIndex = 0;
		Detail::ForEachToken(strPattern.View(), [&](PatternByte iByte) { aBytes[uIndex++] = iByte; });
		return aBytes;
	}

	template <stPatternString strPattern>
	inline constexpr auto Pattern = MakePattern<strPattern>();

	[[nodiscard]] inline std::optional<std::vector<PatternByte>> ParsePattern(std::string_view strPattern)
	{
		std::vector<PatternByte> aBytes;
		if (!Detail::ForEachToken(strPattern, [&](PatternByte iByte) { aBytes.push_back(iByte); }) || aBytes.empty())
			return std::nullopt;
		return aBytes;
	}
}
