#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace Spotify::Json
{
	enum class EType
	{
		Null,
		Bool,
		Number,
		String,
		Array,
		Object,
	};

	struct stValue
	{
		EType eType = EType::Null;
		bool bValue = false;
		double dValue = 0.0;
		std::string strValue;
		std::vector<stValue> aItems;
		std::vector<std::string> aKeys;

		[[nodiscard]] const stValue& operator[](std::string_view strKey) const;
		[[nodiscard]] const stValue& operator[](std::size_t uIndex) const;
		[[nodiscard]] std::size_t Size() const { return aItems.size(); }
		[[nodiscard]] bool IsNull() const { return eType == EType::Null; }
		[[nodiscard]] std::string_view String() const { return eType == EType::String ? std::string_view(strValue) : std::string_view{}; }
		[[nodiscard]] double Number(double dDefault = 0.0) const { return eType == EType::Number ? dValue : dDefault; }
		[[nodiscard]] bool Bool(bool bDefault = false) const { return eType == EType::Bool ? bValue : bDefault; }
	};

	[[nodiscard]] std::optional<stValue> Parse(std::string_view strText);
}
