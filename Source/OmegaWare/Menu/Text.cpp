#include "Menu/Text.hpp"

#include "OmegaLocalization/OmegaLocalization.hpp"

#include <cctype>
#include <functional>
#include <unordered_map>

namespace OmegaWare::Text
{
	namespace
	{
		struct stTextHash
		{
			using is_transparent = void;
			std::size_t operator()(std::string_view strText) const { return std::hash<std::string_view>{}(strText); }
			std::size_t operator()(const std::string& strText) const { return std::hash<std::string_view>{}(strText); }
		};
	}

	bool IsValidUtf8(std::string_view strText)
	{
		for (std::size_t uIndex = 0; uIndex < strText.size();)
		{
			const auto byLead = static_cast<unsigned char>(strText[uIndex]);
			if (byLead < 32 || byLead == 127)
				return false;
			std::size_t uLength = 1;
			if (byLead >= 0xF0 && byLead <= 0xF4)
				uLength = 4;
			else if (byLead >= 0xE0)
				uLength = 3;
			else if (byLead >= 0xC2)
				uLength = 2;
			else if (byLead >= 0x80)
				return false;
			if (uIndex + uLength > strText.size())
				return false;
			for (std::size_t uTail = 1; uTail < uLength; ++uTail)
				if ((static_cast<unsigned char>(strText[uIndex + uTail]) & 0xC0) != 0x80)
					return false;
			uIndex += uLength;
		}
		return true;
	}

	const std::string& Localized(std::string_view strEnglish)
	{
		thread_local std::string t_strLocale;
		thread_local std::unordered_map<std::string, std::string, stTextHash, std::equal_to<>> t_mapText;
		const std::string_view strLocale = OmegaLocalization::GetLocale();
		if (strLocale != t_strLocale)
		{
			t_strLocale = std::string(strLocale);
			t_mapText.clear();
		}
		if (const auto itText = t_mapText.find(strEnglish); itText != t_mapText.end())
			return itText->second;
		std::string strKey = "esp.";
		bool bSeparator = false;
		for (const char chLetter : strEnglish)
		{
			const bool bAlnum = (chLetter >= 'a' && chLetter <= 'z') || (chLetter >= 'A' && chLetter <= 'Z') || (chLetter >= '0' && chLetter <= '9');
			if (bAlnum)
			{
				if (bSeparator)
					strKey.push_back('_');
				strKey.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(chLetter))));
				bSeparator = false;
			}
			else if (strKey.size() > 4)
				bSeparator = true;
		}
		if (t_mapText.size() > 1024)
			t_mapText.clear();
		const std::string strText = OmegaLocalization::Has(strKey) ? std::string(OmegaLocalization::Get(strKey)) : std::string(strEnglish);
		return t_mapText.emplace(std::string(strEnglish), strText).first->second;
	}
}
