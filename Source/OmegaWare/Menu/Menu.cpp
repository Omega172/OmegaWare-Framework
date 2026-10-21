#include "Menu/Menu.hpp"
#include "Menu/Terminal.hpp"
#include "Config/Config.hpp"
#include "Core/Console.hpp"
#include "Core/Framework.hpp"
#include "Features/Feature.hpp"

#include "OmegaGUI/OmegaGUI.hpp"
#include "OmegaLocalization/GUI.hpp"

#include <Windows.h>
#include <shellapi.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <deque>
#include <format>

namespace OmegaWare::Menu
{
	namespace
	{
		using namespace OmegaLocalization::Literals;

		constexpr std::array<float, 8> kScales = { 0.f, 1.f, 1.25f, 1.5f, 1.75f, 2.f, 2.5f, 3.f };

		std::string s_strSelected;
		std::string s_strName;
		std::string s_strPendingDelete;
		std::deque<float> s_aFrameTimes;
		bool s_bOpenAbout = false;

		struct stViewState
		{
			bool* pConsole = nullptr;
			bool* pPerformance = nullptr;
			bool* pWatermark = nullptr;
			int* pTheme = nullptr;
			std::string* pLanguage = nullptr;
		};

		constexpr int kDefaultMenuKey = VK_INSERT;
		constexpr int kDefaultConsoleKey = VK_HOME;
		constexpr int kDefaultUnloadKey = VK_END;

		struct stHotkeyState
		{
			int* pMenu = nullptr;
			int* pConsole = nullptr;
			int* pUnload = nullptr;
		};

		[[nodiscard]] stHotkeyState& HotkeyState()
		{
			static stHotkeyState s_stKeys{
				&Config::Add("keys.menu", kDefaultMenuKey),
				&Config::Add("keys.console", kDefaultConsoleKey),
				&Config::Add("keys.unload", kDefaultUnloadKey),
			};
			return s_stKeys;
		}

		[[nodiscard]] bool IsMouseKey(int iKey)
		{
			return iKey == VK_LBUTTON || iKey == VK_RBUTTON || iKey == VK_MBUTTON;
		}

		void HotkeyRow(std::string_view strLabel, int* pKey, int iDefault, bool bRequired)
		{
			OmegaGUI::KeyCode uKey = static_cast<OmegaGUI::KeyCode>(*pKey);
			if (!OmegaGUI::Hotkey(strLabel, &uKey))
				return;
			if (IsMouseKey(static_cast<int>(uKey)) || (bRequired && uKey == 0))
				uKey = static_cast<OmegaGUI::KeyCode>(iDefault);
			*pKey = static_cast<int>(uKey);
		}

		[[nodiscard]] stViewState& View()
		{
			static stViewState s_stView{
				&Config::Add("menu.show_console", false),
				&Config::Add("menu.show_performance", false),
				&Config::Add("menu.watermark", true),
				&Config::Add("menu.theme", 0),
				&Config::Add("menu.language", std::string{}),
			};
			return s_stView;
		}

		void OpenDataFolder()
		{
			ShellExecuteW(nullptr, L"open", DataDirectory().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
		}

		void Report(bool bSuccess, std::string_view strKey, std::string_view strName)
		{
			if (bSuccess)
				OmegaGUI::PushToast(OmegaGUI::EToastKind::Success, OmegaLocalization::Format(strKey, strName));
			else
				OmegaGUI::PushToast(OmegaGUI::EToastKind::Error, OmegaLocalization::Format("config.failed", Config::GetLastError()));
		}

		bool RequireSelection()
		{
			if (!s_strSelected.empty())
				return true;
			OmegaGUI::PushToast(OmegaGUI::EToastKind::Warning, "config.select_hint"_t);
			return false;
		}

		bool WideButton(std::string_view strLabel, OmegaGUI::EButtonStyle eStyle = OmegaGUI::EButtonStyle::Secondary)
		{
			return OmegaGUI::Button(strLabel, { OmegaGUI::GetContentRegionAvail().x, 0.f }, eStyle);
		}

		void PageHeader(std::string_view strPage, std::string_view strTitle)
		{
			OmegaGUI::Breadcrumbs(std::array<std::string_view, 2>{ kTitle, strPage });
			OmegaGUI::Title(strTitle);
			OmegaGUI::Spacing();
		}

		void ApplyTheme()
		{
			static int s_iApplied = -1;
			int& rTheme = *View().pTheme;
			rTheme = std::clamp(rTheme, 0, 2);
			if (rTheme == s_iApplied)
				return;
			s_iApplied = rTheme;
			OmegaGUI::GetStyle().aColors = (rTheme == 1 ? OmegaGUI::stStyle::ConsoleAmber() : rTheme == 2 ? OmegaGUI::stStyle::ConsoleGreen() : OmegaGUI::stStyle::Console()).aColors;
		}

		void ApplyLanguage()
		{
			std::string& rLanguage = *View().pLanguage;
			if (!rLanguage.empty() && rLanguage != OmegaLocalization::GetLocale() && !OmegaLocalization::SetLocale(rLanguage))
				rLanguage.clear();
		}

		void DrawSettings()
		{
			static int& s_rScale = Config::Add("menu.ui_scale", 0);
			PageHeader("settings", "menu.page_settings"_t);

			OmegaGUI::BeginColumns(2);
			if (OmegaGUI::BeginSection("menu.interface"_l))
			{
				if (OmegaLocalization::GUI::LanguageSelector("menu.language"_l))
					*View().pLanguage = std::string(OmegaLocalization::GetLocale());

				const std::string strAuto(OmegaLocalization::Get("menu.scale_auto"));
				const std::array<std::string_view, kScales.size()> aScaleNames = { strAuto, "100%", "125%", "150%", "175%", "200%", "250%", "300%" };
				s_rScale = std::clamp(s_rScale, 0, static_cast<int>(kScales.size()) - 1);
				OmegaGUI::Combo("menu.ui_scale"_l, &s_rScale, aScaleNames);

				const std::array<std::string_view, 3> aThemes = { "menu.theme_console"_t, "menu.theme_amber"_t, "menu.theme_phosphor"_t };
				OmegaGUI::Combo("menu.theme"_l, View().pTheme, aThemes);
				OmegaGUI::EndSection();
			}
			if (OmegaGUI::BeginSection("menu.section_keys"_l))
			{
				stHotkeyState& rKeys = HotkeyState();
				HotkeyRow("menu.key_menu"_l, rKeys.pMenu, kDefaultMenuKey, true);
				HotkeyRow("menu.key_console"_l, rKeys.pConsole, kDefaultConsoleKey, false);
				HotkeyRow("menu.key_unload"_l, rKeys.pUnload, kDefaultUnloadKey, false);
				OmegaGUI::TextDisabled("menu.keys_hint"_t);
				OmegaGUI::EndSection();
			}
			OmegaGUI::NextColumn();
			if (OmegaGUI::BeginSection("menu.tools"_l))
			{
				OmegaGUI::Toggle("menu.console"_l, View().pConsole);
				OmegaGUI::Toggle("menu.performance"_l, View().pPerformance);
				OmegaGUI::Toggle("menu.watermark"_l, View().pWatermark);
				if (WideButton(Console::IsVisible() ? "menu.console_hide"_l : "menu.console_show"_l))
					Console::Toggle();
				if (WideButton("menu.open_folder"_l))
					OpenDataFolder();
				OmegaGUI::EndSection();
			}
			OmegaGUI::EndColumns();
		}

		void DrawConfig()
		{
			const std::string strDeleteModal = std::format("{}###config.delete_modal", "config.delete"_t);
			const std::string strCurrent = Config::GetCurrent();
			const std::string strDefault = Config::GetDefault();
			PageHeader("config", "menu.page_config"_t);
			OmegaGUI::TextMuted(OmegaLocalization::Format("config.current", strCurrent));
			OmegaGUI::Spacing();

			const std::vector<std::string> aProfiles = Config::List();
			if (std::ranges::find(aProfiles, s_strSelected) == aProfiles.end())
				s_strSelected.clear();

			OmegaGUI::BeginColumns(2);
			if (OmegaGUI::BeginSection("config.profiles"_l))
			{
				if (OmegaGUI::BeginChild("config.list", { 0.f, 140.f * OmegaGUI::GetConfig().flScale }))
				{
					if (aProfiles.empty())
						OmegaGUI::TextDisabled("config.no_profiles"_t);
					for (const std::string& rProfile : aProfiles)
					{
						const std::string strLabel = rProfile == strDefault ? std::format("{} [{}]###{}", rProfile, "config.default_tag"_t, rProfile) : std::format("{}###{}", rProfile, rProfile);
						if (OmegaGUI::Selectable(strLabel, rProfile == s_strSelected))
							s_strSelected = rProfile;
					}
				}
				OmegaGUI::EndChild();
				OmegaGUI::SetNextItemWidth(-1.f);
				OmegaGUI::InputText("##config.name", &s_strName, "config.name_hint"_t);
				OmegaGUI::EndSection();
			}
			OmegaGUI::NextColumn();
			if (OmegaGUI::BeginSection("config.actions"_l))
			{
				if (WideButton("config.save"_l, OmegaGUI::EButtonStyle::Primary))
				{
					const std::string strTarget = !s_strName.empty() ? s_strName : !s_strSelected.empty() ? s_strSelected : strCurrent;
					const bool bSaved = Config::Save(strTarget);
					Report(bSaved, "config.saved", Config::SanitizeName(strTarget));
					if (bSaved)
					{
						s_strSelected = Config::SanitizeName(strTarget);
						s_strName.clear();
					}
				}
				if (WideButton("config.load"_l) && RequireSelection())
					Report(Config::Load(s_strSelected), "config.loaded", s_strSelected);
				if (WideButton("config.set_default"_l) && RequireSelection())
					Report(Config::SetDefault(s_strSelected), "config.default_set", s_strSelected);
				if (WideButton("config.rename"_l) && RequireSelection())
				{
					const bool bRenamed = Config::Rename(s_strSelected, s_strName);
					Report(bRenamed, "config.renamed", Config::SanitizeName(s_strName));
					if (bRenamed)
					{
						s_strSelected = Config::SanitizeName(s_strName);
						s_strName.clear();
					}
				}
				if (WideButton("config.reset"_l))
				{
					Config::ResetToDefaults();
					OmegaGUI::PushToast(OmegaGUI::EToastKind::Info, "config.reset_done"_t);
				}
				if (WideButton("config.delete"_l, OmegaGUI::EButtonStyle::Danger) && RequireSelection())
				{
					s_strPendingDelete = s_strSelected;
					OmegaGUI::OpenPopup(strDeleteModal);
				}
				OmegaGUI::EndSection();
			}
			OmegaGUI::EndColumns();

			if (OmegaGUI::BeginPopupModal(strDeleteModal))
			{
				OmegaGUI::TextUnformatted(OmegaLocalization::Format("config.confirm_delete", s_strPendingDelete));
				if (OmegaGUI::Button("config.yes"_l, {}, OmegaGUI::EButtonStyle::Danger))
				{
					Report(Config::Delete(s_strPendingDelete), "config.deleted", s_strPendingDelete);
					OmegaGUI::CloseCurrentPopup();
				}
				OmegaGUI::SameLine();
				if (OmegaGUI::Button("config.no"_l, {}, OmegaGUI::EButtonStyle::Secondary))
					OmegaGUI::CloseCurrentPopup();
				OmegaGUI::EndPopup();
			}
		}

		template <typename F>
		void Page(std::string_view strLabel, F&& fnDraw)
		{
			if (!OmegaGUI::BeginTabItem(strLabel))
				return;
			if (OmegaGUI::BeginChild("##page", {}, false))
				fnDraw();
			OmegaGUI::EndChild();
			OmegaGUI::EndTabItem();
		}

		void DrawMenuBar()
		{
			if (!OmegaGUI::BeginMenuBar())
				return;
			if (OmegaGUI::BeginMenu("menu.file"_l))
			{
				if (OmegaGUI::MenuItem("menu.save_profile"_l))
					Report(Config::Save(Config::GetCurrent()), "config.saved", Config::GetCurrent());
				if (OmegaGUI::MenuItem("config.reset"_l))
				{
					Config::ResetToDefaults();
					OmegaGUI::PushToast(OmegaGUI::EToastKind::Info, "config.reset_done"_t);
				}
				if (OmegaGUI::MenuItem("menu.open_folder"_l))
					OpenDataFolder();
				if (OmegaGUI::MenuItem("menu.unload"_l, HotkeyName(Hotkeys().iUnload)))
					RequestUnload();
				OmegaGUI::EndMenu();
			}
			if (OmegaGUI::BeginMenu("menu.view"_l))
			{
				OmegaGUI::MenuItem("menu.console"_l, HotkeyName(Hotkeys().iConsole), View().pConsole);
				OmegaGUI::MenuItem("menu.performance"_l, {}, View().pPerformance);
				OmegaGUI::MenuItem("menu.watermark"_l, {}, View().pWatermark);
				OmegaGUI::EndMenu();
			}
			if (OmegaGUI::BeginMenu("menu.help"_l))
			{
				if (OmegaGUI::MenuItem("menu.about"_l))
					s_bOpenAbout = true;
				OmegaGUI::MenuItem("menu.key_menu"_l, HotkeyName(Hotkeys().iMenu), nullptr, false);
				OmegaGUI::MenuItem("menu.key_console"_l, HotkeyName(Hotkeys().iConsole), nullptr, false);
				OmegaGUI::MenuItem("menu.key_unload"_l, HotkeyName(Hotkeys().iUnload), nullptr, false);
				OmegaGUI::EndMenu();
			}
			OmegaGUI::EndMenuBar();
		}

		void DrawAbout(std::string_view strBackend)
		{
			const std::string strAbout = std::format("{}###menu.about_modal", "menu.about"_t);
			if (s_bOpenAbout)
			{
				OmegaGUI::OpenPopup(strAbout);
				s_bOpenAbout = false;
			}
			if (!OmegaGUI::BeginPopupModal(strAbout))
				return;
			OmegaGUI::Title(std::format("{} {}", kTitle, kVersion));
			OmegaGUI::TextMuted(OmegaLocalization::Format("menu.about_game", kGame));
			OmegaGUI::TextMuted(OmegaLocalization::Format("menu.about_backend", strBackend));
			OmegaGUI::TextWrapped("menu.about_text"_t);
			OmegaGUI::Spacing();
			if (OmegaGUI::Button("menu.close"_l, OmegaGUI::vec2{ 120.f, 0.f } * OmegaGUI::GetConfig().flScale))
				OmegaGUI::CloseCurrentPopup();
			OmegaGUI::EndPopup();
		}

		void DrawWatermark(std::string_view strBackend)
		{
			if (!*View().pWatermark)
				return;
			float flAverage = 0.f;
			for (const float flTime : s_aFrameTimes)
				flAverage += flTime;
			flAverage = s_aFrameTimes.empty() ? 0.f : flAverage / static_cast<float>(s_aFrameTimes.size());

			const OmegaGUI::stStyle& rStyle = OmegaGUI::GetStyle();
			const float flScale = OmegaGUI::GetConfig().flScale;
			const std::string strWatermark = std::format("{} {} // {} // {} // {:.0f} fps", kTitle, kVersion, kGame, strBackend, flAverage > 0.f ? 1.f / flAverage : 0.f);
			const OmegaGUI::vec2 vecPrompt = OmegaGUI::CalcTextSize("> ");
			const OmegaGUI::vec2 vecText = OmegaGUI::CalcTextSize(strWatermark);
			const OmegaGUI::vec2 vecPadding = OmegaGUI::vec2{ 12.f, 7.f } * flScale;
			const OmegaGUI::vec2 vecMin = OmegaGUI::vec2{ 12.f, 12.f } * flScale;
			const OmegaGUI::vec2 vecMax = vecMin + vecText + vecPadding * 2.f + OmegaGUI::vec2{ vecPrompt.x, 0.f };

			OmegaGUI::clDrawList& rList = OmegaGUI::GetBackgroundDrawList();
			rList.AddShadowRect(vecMin, vecMax, rStyle.Get(OmegaGUI::EColor::Shadow), 16.f * flScale, rStyle.flRoundingSmall, { 0.f, 4.f * flScale });
			rList.AddRectFilled(vecMin, vecMax, rStyle.Get(OmegaGUI::EColor::Surface).WithAlpha(235), rStyle.flRoundingSmall);
			rList.AddRect(vecMin, vecMax, rStyle.Get(OmegaGUI::EColor::Border), rStyle.flRoundingSmall);
			rList.AddRectFilled(vecMin, { vecMax.x, vecMin.y + 2.f }, rStyle.Get(OmegaGUI::EColor::Accent), rStyle.flRoundingSmall, OmegaGUI::ECorner::Top);
			rList.AddText(vecMin + vecPadding, rStyle.Get(OmegaGUI::EColor::Accent), "> ");
			rList.AddText(vecMin + vecPadding + OmegaGUI::vec2{ vecPrompt.x, 0.f }, rStyle.Get(OmegaGUI::EColor::Text), strWatermark);
		}

		void DrawPerformance(std::string_view strBackend)
		{
			bool& rOpen = *View().pPerformance;
			if (!rOpen)
				return;
			const float flScale = OmegaGUI::GetConfig().flScale;
			OmegaGUI::SetNextWindowPos({ OmegaGUI::GetDisplaySize().x - 340.f * flScale, 80.f * flScale });
			OmegaGUI::SetNextWindowSize(OmegaGUI::vec2{ 300.f, 210.f } * flScale);
			if (OmegaGUI::Begin(std::format("{}###menu.perf", "menu.performance"_t), &rOpen))
			{
				std::vector<float> aValues(s_aFrameTimes.begin(), s_aFrameTimes.end());
				for (float& rValue : aValues)
					rValue *= 1000.f;
				OmegaGUI::LabelText("menu.perf_frame"_t, std::format("{:.2f} ms", aValues.empty() ? 0.f : aValues.back()));
				OmegaGUI::LabelText("menu.perf_backend"_t, strBackend);
				OmegaGUI::PlotLines("##frametimes", aValues, 0.f, 0.f, { 0.f, -1.f });
			}
			OmegaGUI::End();
		}
	}

	void Initialize()
	{
		Terminal::Initialize();
	}

	void Shutdown()
	{
		Terminal::Shutdown();
	}

	void Draw(std::string_view strBackend)
	{
		ApplyLanguage();
		ApplyTheme();

		s_aFrameTimes.push_back(OmegaGUI::GetDeltaTime());
		while (s_aFrameTimes.size() > 120)
			s_aFrameTimes.pop_front();

		DrawWatermark(strBackend);

		static float s_flLastScale = 0.f;
		const float flScale = OmegaGUI::GetConfig().flScale;
		const bool bScaleChanged = s_flLastScale > 0.f && s_flLastScale != flScale;
		s_flLastScale = flScale;
		const OmegaGUI::ECondition eCondition = bScaleChanged ? OmegaGUI::ECondition::Always : OmegaGUI::ECondition::FirstUse;
		OmegaGUI::SetNextWindowPos(OmegaGUI::vec2{ 80.f, 80.f } * flScale, eCondition);
		OmegaGUI::SetNextWindowSize(OmegaGUI::vec2{ 780.f, 540.f } * flScale, eCondition);
		if (OmegaGUI::Begin(kTitle, nullptr, OmegaGUI::EWindowFlags::MenuBar | OmegaGUI::EWindowFlags::CloseHidesMenu))
		{
			DrawMenuBar();
			if (OmegaGUI::BeginTabBar("##pages", OmegaGUI::ETabBarFlags::Vertical))
			{
				OmegaGUI::TabBarSection("menu.section_features"_t);
				const auto aFeatures = Features::All();
				if (aFeatures.empty())
					Page("menu.no_features"_l, [] { OmegaGUI::TextDisabled("feature.none"_t); });
				for (clFeature* pFeature : aFeatures)
					Page(std::format("{}###feature.{}", Features::DisplayName(*pFeature), pFeature->GetName()), [pFeature] { Features::DrawPage(*pFeature); });

				OmegaGUI::TabBarSection("menu.section_framework"_t);
				Page("menu.page_settings"_l, DrawSettings);
				Page("menu.page_config"_l, DrawConfig);
				OmegaGUI::EndTabBar();
			}
			DrawAbout(strBackend);
		}
		OmegaGUI::End();
		OmegaGUI::ControllerLegend();

		DrawPerformance(strBackend);
		Terminal::Draw(View().pConsole);
	}

	void ToggleConsole()
	{
		*View().pConsole = !*View().pConsole;
	}

	stHotkeys Hotkeys()
	{
		const stHotkeyState& rKeys = HotkeyState();
		return { *rKeys.pMenu, *rKeys.pConsole, *rKeys.pUnload };
	}

	std::string HotkeyName(int iKey)
	{
		if (iKey <= 0)
			return std::string(OmegaLocalization::Get("menu.key_none"));
		std::string strName = OmegaGUI::KeyName(static_cast<OmegaGUI::KeyCode>(iKey));
		for (char& rChar : strName)
			rChar = static_cast<char>(std::tolower(static_cast<unsigned char>(rChar)));
		return strName;
	}

	float ResolveScale(float flDisplayHeight)
	{
		const int iScale = std::clamp(Config::Add("menu.ui_scale", 0), 0, static_cast<int>(kScales.size()) - 1);
		if (iScale > 0)
			return kScales[iScale];
		if (flDisplayHeight <= 0.f)
			return 1.f;
		return std::clamp(std::round(flDisplayHeight / 1080.f * 4.f) / 4.f, 1.f, 3.f);
	}
}
