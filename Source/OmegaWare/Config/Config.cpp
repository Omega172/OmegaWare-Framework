#include "Config/Config.hpp"

#include "OmegaGUI/OmegaGUI.hpp"
#include "OmegaLocalization/OmegaLocalization.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
#include <format>
#include <fstream>
#include <map>
#include <memory>
#include <mutex>
#include <stdexcept>

namespace OmegaWare::Config
{
	namespace
	{
		struct stEntry
		{
			Value_t valCurrent;
			Value_t valDefault;
		};

		std::recursive_mutex g_mtxConfig;
		std::filesystem::path g_pathDirectory;
		std::map<std::string, std::unique_ptr<stEntry>, std::less<>> g_mapValues;
		nlohmann::json g_jsonLoaded = nlohmann::json::object();
		std::string g_strCurrent(kDefaultProfile);
		std::string g_strDefault(kDefaultProfile);
		std::string g_strLastError;

		[[nodiscard]] std::filesystem::path ProfileDirectory()
		{
			return g_pathDirectory / "Profiles";
		}

		[[nodiscard]] std::filesystem::path ProfilePath(std::string_view strName)
		{
			return ProfileDirectory() / std::filesystem::path(std::u8string(strName.begin(), strName.end()) + u8".json");
		}

		[[nodiscard]] std::filesystem::path ManifestPath()
		{
			return g_pathDirectory / "Manifest.json";
		}

		[[nodiscard]] std::string_view ModeName(EKeybindMode eMode)
		{
			switch (eMode)
			{
			case EKeybindMode::AlwaysOn: return "always";
			case EKeybindMode::Hold: return "hold";
			default: return "toggle";
			}
		}

		[[nodiscard]] EKeybindMode ModeFromName(std::string_view strName)
		{
			if (strName == "always")
				return EKeybindMode::AlwaysOn;
			if (strName == "hold")
				return EKeybindMode::Hold;
			return EKeybindMode::Toggle;
		}

		[[nodiscard]] nlohmann::json ToJson(const Value_t& rValue)
		{
			return std::visit([](const auto& rInner) -> nlohmann::json {
				using T = std::decay_t<decltype(rInner)>;
				if constexpr (std::is_same_v<T, OmegaGUI::color>)
					return rInner.uPacked;
				else if constexpr (std::is_same_v<T, stKeybind>)
					return { { "key", rInner.uKey }, { "mode", ModeName(rInner.eMode) }, { "notify", rInner.bNotify } };
				else
					return rInner;
			}, rValue);
		}

		bool FromJson(const nlohmann::json& rJson, Value_t& rValue)
		{
			return std::visit([&](auto& rInner) -> bool {
				using T = std::decay_t<decltype(rInner)>;
				if constexpr (std::is_same_v<T, bool>)
				{
					if (!rJson.is_boolean())
						return false;
					rInner = rJson.get<bool>();
				}
				else if constexpr (std::is_same_v<T, int>)
				{
					if (!rJson.is_number_integer())
						return false;
					rInner = rJson.get<int>();
				}
				else if constexpr (std::is_same_v<T, float>)
				{
					if (!rJson.is_number())
						return false;
					rInner = rJson.get<float>();
				}
				else if constexpr (std::is_same_v<T, std::string>)
				{
					if (!rJson.is_string())
						return false;
					rInner = rJson.get<std::string>();
				}
				else if constexpr (std::is_same_v<T, OmegaGUI::color>)
				{
					if (!rJson.is_number_integer())
						return false;
					rInner = OmegaGUI::color(rJson.get<std::uint32_t>());
				}
				else if constexpr (std::is_same_v<T, stKeybind>)
				{
					if (!rJson.is_object() || !rJson.contains("key") || !rJson["key"].is_number_integer())
						return false;
					rInner.uKey = rJson["key"].get<OmegaGUI::KeyCode>();
					rInner.eMode = rJson.contains("mode") && rJson["mode"].is_string() ? ModeFromName(rJson["mode"].get<std::string>()) : EKeybindMode::Toggle;
					rInner.bNotify = rJson.contains("notify") && rJson["notify"].is_boolean() && rJson["notify"].get<bool>();
					rInner.bActive = rInner.eMode == EKeybindMode::AlwaysOn;
				}
				return true;
			}, rValue);
		}

		bool Fail(std::string strError)
		{
			g_strLastError = std::move(strError);
			return false;
		}

		bool WriteJson(const std::filesystem::path& pathFile, const nlohmann::json& rJson)
		{
			std::error_code errCode;
			std::filesystem::create_directories(pathFile.parent_path(), errCode);
			std::filesystem::path pathTemp = pathFile;
			pathTemp += ".tmp";
			{
				std::ofstream fileOut(pathTemp, std::ios::out | std::ios::trunc | std::ios::binary);
				if (!fileOut)
					return Fail(std::format("could not write {}", pathTemp.string()));
				fileOut << rJson.dump(4);
				if (!fileOut)
					return Fail(std::format("could not write {}", pathTemp.string()));
			}
			std::filesystem::rename(pathTemp, pathFile, errCode);
			if (errCode)
				return Fail(std::format("could not replace {}: {}", pathFile.string(), errCode.message()));
			return true;
		}

		bool ReadJson(const std::filesystem::path& pathFile, nlohmann::json& rJson)
		{
			std::ifstream fileIn(pathFile, std::ios::in | std::ios::binary);
			if (!fileIn)
				return Fail(std::format("could not open {}", pathFile.string()));
			rJson = nlohmann::json::parse(fileIn, nullptr, false);
			if (rJson.is_discarded())
				return Fail(std::format("{} is not valid json", pathFile.string()));
			return true;
		}

		void WriteManifest()
		{
			WriteJson(ManifestPath(), { { "default", g_strDefault } });
		}

		void ApplyLoaded()
		{
			for (auto& [strKey, pEntry] : g_mapValues)
			{
				pEntry->valCurrent = pEntry->valDefault;
				if (g_jsonLoaded.contains(strKey))
					FromJson(g_jsonLoaded[strKey], pEntry->valCurrent);
			}
		}
	}

	void Initialize(const std::filesystem::path& pathDirectory)
	{
		std::scoped_lock lckConfig(g_mtxConfig);
		g_pathDirectory = pathDirectory;
		std::error_code errCode;
		std::filesystem::create_directories(ProfileDirectory(), errCode);

		nlohmann::json jsonManifest;
		if (std::filesystem::exists(ManifestPath(), errCode) && ReadJson(ManifestPath(), jsonManifest) && jsonManifest.contains("default") && jsonManifest["default"].is_string())
		{
			const std::string strDefault = SanitizeName(jsonManifest["default"].get<std::string>());
			if (!strDefault.empty())
				g_strDefault = strDefault;
		}

		g_strCurrent = g_strDefault;
		if (std::filesystem::exists(ProfilePath(g_strDefault), errCode))
			Load(g_strDefault);
	}

	Value_t& AddValue(std::string_view strKey, Value_t valDefault)
	{
		std::scoped_lock lckConfig(g_mtxConfig);
		if (const auto itEntry = g_mapValues.find(strKey); itEntry != g_mapValues.end())
		{
			if (itEntry->second->valDefault.index() != valDefault.index())
				throw std::logic_error(std::format("config key \"{}\" was registered twice with different types", strKey));
			return itEntry->second->valCurrent;
		}

		const std::string strName(strKey);
		auto pEntry = std::make_unique<stEntry>(valDefault, valDefault);
		if (g_jsonLoaded.contains(strName))
			FromJson(g_jsonLoaded[strName], pEntry->valCurrent);
		Value_t& rValue = pEntry->valCurrent;
		g_mapValues.emplace(strName, std::move(pEntry));
		return rValue;
	}

	void UpdateKeybinds()
	{
		std::scoped_lock lckConfig(g_mtxConfig);
		for (auto& [strKey, pEntry] : g_mapValues)
		{
			stKeybind* pKeybind = std::get_if<stKeybind>(&pEntry->valCurrent);
			if (!pKeybind || !OmegaGUI::UpdateHotkey(*pKeybind) || !pKeybind->bNotify)
				continue;
			const std::string strText = OmegaLocalization::Format(pKeybind->bActive ? "keybind.toast_on" : "keybind.toast_off", OmegaLocalization::Get(strKey));
			OmegaGUI::PushToast(pKeybind->bActive ? OmegaGUI::EToastKind::Success : OmegaGUI::EToastKind::Info, strText, 1.5f);
		}
	}

	bool Save(std::string_view strName)
	{
		std::scoped_lock lckConfig(g_mtxConfig);
		const std::string strProfile = SanitizeName(strName);
		if (strProfile.empty())
			return Fail("the profile name is empty");

		nlohmann::json jsonValues = g_jsonLoaded;
		for (const auto& [strKey, pEntry] : g_mapValues)
			jsonValues[strKey] = ToJson(pEntry->valCurrent);

		if (!WriteJson(ProfilePath(strProfile), { { "version", 1 }, { "values", jsonValues } }))
			return false;
		g_jsonLoaded = std::move(jsonValues);
		g_strCurrent = strProfile;
		return true;
	}

	bool Load(std::string_view strName)
	{
		std::scoped_lock lckConfig(g_mtxConfig);
		const std::string strProfile = SanitizeName(strName);
		if (strProfile.empty())
			return Fail("the profile name is empty");

		nlohmann::json jsonFile;
		if (!ReadJson(ProfilePath(strProfile), jsonFile))
			return false;
		if (!jsonFile.is_object() || !jsonFile.contains("values") || !jsonFile["values"].is_object())
			return Fail(std::format("profile \"{}\" has no values", strProfile));

		g_jsonLoaded = jsonFile["values"];
		ApplyLoaded();
		g_strCurrent = strProfile;
		return true;
	}

	bool Delete(std::string_view strName)
	{
		std::scoped_lock lckConfig(g_mtxConfig);
		const std::string strProfile = SanitizeName(strName);
		std::error_code errCode;
		if (strProfile.empty() || !std::filesystem::remove(ProfilePath(strProfile), errCode))
			return Fail(std::format("could not delete profile \"{}\"", strProfile));
		if (g_strDefault == strProfile)
		{
			g_strDefault = kDefaultProfile;
			WriteManifest();
		}
		if (g_strCurrent == strProfile)
			g_strCurrent = g_strDefault;
		return true;
	}

	bool Rename(std::string_view strOldName, std::string_view strNewName)
	{
		std::scoped_lock lckConfig(g_mtxConfig);
		const std::string strOld = SanitizeName(strOldName);
		const std::string strNew = SanitizeName(strNewName);
		std::error_code errCode;
		if (strOld.empty() || strNew.empty())
			return Fail("the profile name is empty");
		if (!std::filesystem::exists(ProfilePath(strOld), errCode))
			return Fail(std::format("profile \"{}\" does not exist", strOld));
		if (std::filesystem::exists(ProfilePath(strNew), errCode))
			return Fail(std::format("profile \"{}\" already exists", strNew));
		std::filesystem::rename(ProfilePath(strOld), ProfilePath(strNew), errCode);
		if (errCode)
			return Fail(std::format("could not rename \"{}\": {}", strOld, errCode.message()));
		if (g_strDefault == strOld)
		{
			g_strDefault = strNew;
			WriteManifest();
		}
		if (g_strCurrent == strOld)
			g_strCurrent = strNew;
		return true;
	}

	bool SetDefault(std::string_view strName)
	{
		std::scoped_lock lckConfig(g_mtxConfig);
		const std::string strProfile = SanitizeName(strName);
		if (strProfile.empty())
			return Fail("the profile name is empty");
		g_strDefault = strProfile;
		WriteManifest();
		return true;
	}

	void ResetToDefaults()
	{
		std::scoped_lock lckConfig(g_mtxConfig);
		for (auto& [strKey, pEntry] : g_mapValues)
			pEntry->valCurrent = pEntry->valDefault;
	}

	std::vector<std::string> List()
	{
		std::scoped_lock lckConfig(g_mtxConfig);
		std::vector<std::string> aProfiles;
		std::error_code errCode;
		for (const auto& rEntry : std::filesystem::directory_iterator(ProfileDirectory(), errCode))
		{
			if (rEntry.is_regular_file(errCode) && rEntry.path().extension() == ".json")
			{
				const std::u8string strStem = rEntry.path().stem().u8string();
				aProfiles.emplace_back(strStem.begin(), strStem.end());
			}
		}
		std::ranges::sort(aProfiles);
		return aProfiles;
	}

	std::string GetDefault()
	{
		std::scoped_lock lckConfig(g_mtxConfig);
		return g_strDefault;
	}

	std::string GetCurrent()
	{
		std::scoped_lock lckConfig(g_mtxConfig);
		return g_strCurrent;
	}

	std::string GetLastError()
	{
		std::scoped_lock lckConfig(g_mtxConfig);
		return g_strLastError;
	}

	std::string SanitizeName(std::string_view strName)
	{
		std::string strResult;
		for (const char chValue : strName)
		{
			const auto byValue = static_cast<unsigned char>(chValue);
			if (std::isalnum(byValue) || chValue == ' ' || chValue == '-' || chValue == '_' || chValue == '.')
				strResult += chValue;
		}
		const auto itFirst = strResult.find_first_not_of(" .");
		if (itFirst == std::string::npos)
			return {};
		strResult = strResult.substr(itFirst, strResult.find_last_not_of(" .") - itFirst + 1);
		if (strResult.size() > 64)
			strResult.resize(64);
		return strResult;
	}

	const std::filesystem::path& GetDirectory()
	{
		return g_pathDirectory;
	}
}
