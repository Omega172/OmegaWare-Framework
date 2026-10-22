#include "Json.hpp"

#include <charconv>
#include <cstdint>

namespace Spotify::Json
{
	namespace
	{
		const stValue s_stNull;

		class clParser
		{
		public:
			explicit clParser(std::string_view strText) : m_strText(strText) {}

			bool ParseDocument(stValue& rValue)
			{
				if (!ParseValue(rValue, 0))
					return false;
				SkipSpace();
				return m_uPos == m_strText.size();
			}

		private:
			static constexpr int kMaxDepth = 64;

			void SkipSpace()
			{
				while (m_uPos < m_strText.size() && (m_strText[m_uPos] == ' ' || m_strText[m_uPos] == '\t' || m_strText[m_uPos] == '\r' || m_strText[m_uPos] == '\n'))
					++m_uPos;
			}

			bool Consume(char chExpected)
			{
				SkipSpace();
				if (m_uPos >= m_strText.size() || m_strText[m_uPos] != chExpected)
					return false;
				++m_uPos;
				return true;
			}

			bool ConsumeWord(std::string_view strWord)
			{
				if (m_strText.substr(m_uPos, strWord.size()) != strWord)
					return false;
				m_uPos += strWord.size();
				return true;
			}

			bool ParseValue(stValue& rValue, int iDepth)
			{
				if (iDepth > kMaxDepth)
					return false;
				SkipSpace();
				if (m_uPos >= m_strText.size())
					return false;
				const char chFirst = m_strText[m_uPos];
				if (chFirst == '{')
					return ParseObject(rValue, iDepth);
				if (chFirst == '[')
					return ParseArray(rValue, iDepth);
				if (chFirst == '"')
				{
					rValue.eType = EType::String;
					return ParseString(rValue.strValue);
				}
				if (ConsumeWord("true"))
				{
					rValue.eType = EType::Bool;
					rValue.bValue = true;
					return true;
				}
				if (ConsumeWord("false"))
				{
					rValue.eType = EType::Bool;
					rValue.bValue = false;
					return true;
				}
				if (ConsumeWord("null"))
				{
					rValue.eType = EType::Null;
					return true;
				}
				return ParseNumber(rValue);
			}

			bool ParseObject(stValue& rValue, int iDepth)
			{
				rValue.eType = EType::Object;
				++m_uPos;
				if (Consume('}'))
					return true;
				do
				{
					SkipSpace();
					std::string strKey;
					if (m_uPos >= m_strText.size() || m_strText[m_uPos] != '"' || !ParseString(strKey) || !Consume(':'))
						return false;
					stValue stItem;
					if (!ParseValue(stItem, iDepth + 1))
						return false;
					rValue.aKeys.push_back(std::move(strKey));
					rValue.aItems.push_back(std::move(stItem));
				} while (Consume(','));
				return Consume('}');
			}

			bool ParseArray(stValue& rValue, int iDepth)
			{
				rValue.eType = EType::Array;
				++m_uPos;
				if (Consume(']'))
					return true;
				do
				{
					stValue stItem;
					if (!ParseValue(stItem, iDepth + 1))
						return false;
					rValue.aItems.push_back(std::move(stItem));
				} while (Consume(','));
				return Consume(']');
			}

			bool ParseHex(std::uint32_t& rCode)
			{
				if (m_uPos + 4 > m_strText.size())
					return false;
				const char* pBegin = m_strText.data() + m_uPos;
				const auto stResult = std::from_chars(pBegin, pBegin + 4, rCode, 16);
				if (stResult.ec != std::errc{} || stResult.ptr != pBegin + 4)
					return false;
				m_uPos += 4;
				return true;
			}

			static void AppendUtf8(std::string& rOut, std::uint32_t uCode)
			{
				if (uCode < 0x80)
					rOut += static_cast<char>(uCode);
				else if (uCode < 0x800)
				{
					rOut += static_cast<char>(0xC0 | (uCode >> 6));
					rOut += static_cast<char>(0x80 | (uCode & 0x3F));
				}
				else if (uCode < 0x10000)
				{
					rOut += static_cast<char>(0xE0 | (uCode >> 12));
					rOut += static_cast<char>(0x80 | ((uCode >> 6) & 0x3F));
					rOut += static_cast<char>(0x80 | (uCode & 0x3F));
				}
				else
				{
					rOut += static_cast<char>(0xF0 | (uCode >> 18));
					rOut += static_cast<char>(0x80 | ((uCode >> 12) & 0x3F));
					rOut += static_cast<char>(0x80 | ((uCode >> 6) & 0x3F));
					rOut += static_cast<char>(0x80 | (uCode & 0x3F));
				}
			}

			bool ParseString(std::string& rOut)
			{
				++m_uPos;
				while (m_uPos < m_strText.size())
				{
					const char chCurrent = m_strText[m_uPos++];
					if (chCurrent == '"')
						return true;
					if (chCurrent != '\\')
					{
						rOut += chCurrent;
						continue;
					}
					if (m_uPos >= m_strText.size())
						return false;
					const char chEscape = m_strText[m_uPos++];
					switch (chEscape)
					{
					case '"': rOut += '"'; break;
					case '\\': rOut += '\\'; break;
					case '/': rOut += '/'; break;
					case 'b': rOut += '\b'; break;
					case 'f': rOut += '\f'; break;
					case 'n': rOut += '\n'; break;
					case 'r': rOut += '\r'; break;
					case 't': rOut += '\t'; break;
					case 'u':
					{
						std::uint32_t uCode = 0;
						if (!ParseHex(uCode))
							return false;
						if (uCode >= 0xD800 && uCode <= 0xDBFF && m_strText.substr(m_uPos, 2) == "\\u")
						{
							m_uPos += 2;
							std::uint32_t uLow = 0;
							if (!ParseHex(uLow))
								return false;
							if (uLow >= 0xDC00 && uLow <= 0xDFFF)
								uCode = 0x10000 + ((uCode - 0xD800) << 10) + (uLow - 0xDC00);
							else
							{
								AppendUtf8(rOut, 0xFFFD);
								uCode = uLow;
							}
						}
						if (uCode >= 0xD800 && uCode <= 0xDFFF)
							uCode = 0xFFFD;
						AppendUtf8(rOut, uCode);
						break;
					}
					default: return false;
					}
				}
				return false;
			}

			bool ParseNumber(stValue& rValue)
			{
				const char* pBegin = m_strText.data() + m_uPos;
				const char* pEnd = m_strText.data() + m_strText.size();
				double dValue = 0.0;
				const auto stResult = std::from_chars(pBegin, pEnd, dValue);
				if (stResult.ec != std::errc{} || stResult.ptr == pBegin)
					return false;
				m_uPos += static_cast<std::size_t>(stResult.ptr - pBegin);
				rValue.eType = EType::Number;
				rValue.dValue = dValue;
				return true;
			}

			std::string_view m_strText;
			std::size_t m_uPos = 0;
		};
	}

	const stValue& stValue::operator[](std::string_view strKey) const
	{
		if (eType != EType::Object)
			return s_stNull;
		for (std::size_t i = 0; i < aKeys.size(); ++i)
			if (aKeys[i] == strKey)
				return aItems[i];
		return s_stNull;
	}

	const stValue& stValue::operator[](std::size_t uIndex) const
	{
		return eType == EType::Array && uIndex < aItems.size() ? aItems[uIndex] : s_stNull;
	}

	std::optional<stValue> Parse(std::string_view strText)
	{
		stValue stRoot;
		clParser clParse(strText);
		if (!clParse.ParseDocument(stRoot))
			return std::nullopt;
		return stRoot;
	}
}
