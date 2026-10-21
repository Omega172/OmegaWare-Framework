#pragma once

#include "OmegaGUI/Core/Color.hpp"
#include "OmegaGUI/Input/Input.hpp"

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace OmegaWare::Config
{
	enum class EKeybindMode : std::uint8_t
	{
		AlwaysOn,
		Hold,
		Toggle,
	};

	struct stKeybind
	{
		OmegaGUI::KeyCode uKey = 0;
		EKeybindMode eMode = EKeybindMode::Toggle;
		bool bActive = false;
	};

	using Value_t = std::variant<bool, int, float, std::string, OmegaGUI::color, stKeybind>;

	inline constexpr std::string_view kDefaultProfile = "default";

	void Initialize(const std::filesystem::path& pathDirectory);

	[[nodiscard]] Value_t& AddValue(std::string_view strKey, Value_t valDefault);

	template <typename T>
	[[nodiscard]] T& Add(std::string_view strKey, T tDefault)
	{
		return std::get<T>(AddValue(strKey, Value_t(std::move(tDefault))));
	}

	void UpdateKeybinds();

	bool Save(std::string_view strName);
	bool Load(std::string_view strName);
	bool Delete(std::string_view strName);
	bool Rename(std::string_view strOldName, std::string_view strNewName);
	bool SetDefault(std::string_view strName);
	void ResetToDefaults();

	[[nodiscard]] std::vector<std::string> List();
	[[nodiscard]] std::string GetDefault();
	[[nodiscard]] std::string GetCurrent();
	[[nodiscard]] std::string GetLastError();
	[[nodiscard]] std::string SanitizeName(std::string_view strName);
	[[nodiscard]] const std::filesystem::path& GetDirectory();
}
