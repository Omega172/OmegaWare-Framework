#include "Features/Feature.hpp"
#include "Config/Config.hpp"
#include "Menu/Widgets.hpp"

#include "OmegaGUI/OmegaGUI.hpp"
#include "OmegaLocalization/OmegaLocalization.hpp"

#include <algorithm>
#include <array>
#include <utility>
#include <atomic>
#include <cmath>
#include <format>

namespace OmegaWare
{
	namespace
	{
		class clExampleFeature final : public clFeature
		{
		public:
			[[nodiscard]] std::string_view GetName() const override { return "example"; }

			bool Setup() override
			{
				m_pCrosshair = &Config::Add("example.crosshair", Config::stKeybind{});
				m_pCrosshairSize = &Config::Add("example.crosshair_size", 8.f);
				m_pDepthProbe = &Config::Add("example.depth_probe", false);
				m_pDepthProbeDepth = &Config::Add("example.depth_probe_depth", .999f);
				return true;
			}

			void Run() override
			{
				++m_uTicks;
			}

			void Render() override
			{
				if (*m_pDepthProbe)
					DrawDepthProbe();
				if (!m_pCrosshair->bActive)
					return;
				const float flScale = OmegaGUI::GetConfig().flScale;
				const OmegaGUI::color colCross = OmegaGUI::GetStyle().Get(OmegaGUI::EColor::Accent);
				const OmegaGUI::vec2 vecCenter = (OmegaGUI::GetDisplaySize() * .5f).Floor();
				const float flGap = std::round(3.f * flScale);
				const float flLength = std::round(*m_pCrosshairSize * flScale);
				const float flWidth = std::max(1.f, std::round(flScale));
				const float flOutline = std::max(1.f, std::round(flScale));
				const std::array<std::pair<OmegaGUI::vec2, OmegaGUI::vec2>, 4> aArms = { {
					{ vecCenter - OmegaGUI::vec2{ flGap + flLength, 0.f }, vecCenter + OmegaGUI::vec2{ -flGap, flWidth } },
					{ vecCenter + OmegaGUI::vec2{ flGap + flWidth, 0.f }, vecCenter + OmegaGUI::vec2{ flGap + flWidth + flLength, flWidth } },
					{ vecCenter - OmegaGUI::vec2{ 0.f, flGap + flLength }, vecCenter + OmegaGUI::vec2{ flWidth, -flGap } },
					{ vecCenter + OmegaGUI::vec2{ 0.f, flGap + flWidth }, vecCenter + OmegaGUI::vec2{ flWidth, flGap + flWidth + flLength } },
				} };
				OmegaGUI::clDrawList& rList = OmegaGUI::GetBackgroundDrawList();
				for (const auto& [vecMin, vecMax] : aArms)
					rList.AddRectFilled(vecMin - OmegaGUI::vec2{ flOutline, flOutline }, vecMax + OmegaGUI::vec2{ flOutline, flOutline }, OmegaGUI::color(0, 0, 0, 255));
				for (const auto& [vecMin, vecMax] : aArms)
					rList.AddRectFilled(vecMin, vecMax, colCross);
			}

			void DrawMenu() override
			{
				using namespace OmegaLocalization::Literals;
				OmegaGUI::BeginColumns(2);
				if (OmegaGUI::BeginSection("example.section_status"_l))
				{
					OmegaGUI::TextMuted(OmegaLocalization::Format("example.uptime", m_uTicks.load() / 10));
					OmegaGUI::EndSection();
				}
				OmegaGUI::NextColumn();
				if (OmegaGUI::BeginSection("example.section_crosshair"_l))
				{
					Widgets::Keybind("example.crosshair"_l, "example.crosshair", *m_pCrosshair);
					OmegaGUI::SliderFloat("example.crosshair_size"_l, m_pCrosshairSize, 2.f, 40.f, "{:.0f}");
					OmegaGUI::EndSection();
				}
				if (OmegaGUI::BeginSection("example.section_depth"_l))
				{
					OmegaGUI::Checkbox("example.depth_probe"_l, m_pDepthProbe);
					OmegaGUI::SliderFloat("example.depth_probe_depth"_l, m_pDepthProbeDepth, 0.f, 1.f, "{:.4f}");
					OmegaGUI::EndSection();
				}
				OmegaGUI::EndColumns();
			}

		private:
			void DrawDepthProbe()
			{
				OmegaGUI::clWorldDrawList& rWorld = OmegaGUI::GetWorldDrawList();
				rWorld.SetCamera(OmegaGUI::mat4{}, OmegaGUI::GetDisplaySize());
				const float flDepth = std::clamp(*m_pDepthProbeDepth, 0.f, 1.f);
				const OmegaGUI::color colVisible = OmegaGUI::GetStyle().Get(OmegaGUI::EColor::Accent).WithAlpha(90);
				rWorld.AddQuadFilled({ -1.f, -1.f, flDepth }, { 1.f, -1.f, flDepth }, { 1.f, 1.f, flDepth }, { -1.f, 1.f, flDepth }, colVisible, OmegaGUI::EDepthMode::XRay, { OmegaGUI::color(255, 80, 80, 90), 1.f });
			}

			Config::stKeybind* m_pCrosshair = nullptr;
			float* m_pCrosshairSize = nullptr;
			bool* m_pDepthProbe = nullptr;
			float* m_pDepthProbeDepth = nullptr;
			std::atomic<unsigned int> m_uTicks = 0;
		};

		clExampleFeature g_clExampleFeature;
	}
}
