#pragma once

#include <string>
#include <string_view>

namespace OmegaWare::Text
{
	[[nodiscard]] bool IsValidUtf8(std::string_view strText);
	[[nodiscard]] const std::string& Localized(std::string_view strEnglish);
}
