#include "OmegaWarePlugin.hpp"

#include <array>
#include <format>

namespace
{
	using namespace OmegaWare;

	constexpr const char* kBanner = "Hello, World!";
	constexpr std::array<const char*, 3> kCorners = { "top right", "top left", "bottom left" };

	class clBannerFeature final : public Plugin::clFeature
	{
	public:
		[[nodiscard]] const char* GetName() const override { return "plugin banner"; }

		bool Setup() override
		{
			m_pVisible = Plugin::Api().ConfigBool("example_plugin.visible", true);
			m_pRadius = Plugin::Api().ConfigFloat("example_plugin.radius", 6.f);
			m_pCorner = Plugin::Api().ConfigInt("example_plugin.corner", 0);
			m_pColor = Plugin::Api().ConfigColor("example_plugin.color", Plugin::Api().AccentColor());
			m_pKeybind = Plugin::Api().ConfigKeybind("example_plugin.keybind", 0);
			Plugin::Loc::Add("en", "example_plugin.greeting", "hello from the example plugin");
			Plugin::Log(Plugin::ELogLevel::Info, std::format("banner feature set up for {}", Plugin::Api().szGame));
			return true;
		}

		void Run() override
		{
			++m_uTicks;
		}

		void Render() override
		{
			if (!*m_pVisible || Plugin::Api().KeybindActive(m_pKeybind))
				return;
			float flWidth = 0.f;
			float flHeight = 0.f;
			Plugin::Api().GetDisplaySize(&flWidth, &flHeight);
			const std::uint32_t uAccent = *m_pColor;
			const float flX = *m_pCorner == 0 ? flWidth - 260.f : 20.f;
			const float flY = *m_pCorner == 2 ? flHeight - 40.f : 12.f;
			Plugin::Api().DrawRectFilled(flX - 8.f, flY - 4.f, flWidth - 12.f, flY + 22.f, Plugin::Rgba(0, 0, 0, 170));
			Plugin::Api().DrawCircle(flX + 4.f, flY + 9.f, *m_pRadius, uAccent, 2.f);
			Plugin::Api().DrawText(flX + 16.f, flY, uAccent, kBanner);
		}

		void DrawMenu() override
		{
			if (Plugin::Api().BeginSection("banner"))
			{
				Plugin::Api().Toggle("show banner", m_pVisible);
				Plugin::Api().SliderFloat("dot radius", m_pRadius, 2.f, 10.f, "{:.0f}");
				Plugin::Api().Combo("corner", m_pCorner, kCorners.data(), static_cast<int>(kCorners.size()));
				Plugin::Api().ColorEdit("color", m_pColor, false);
				Plugin::Api().Keybind("hide key", m_pKeybind);
				Plugin::Api().TextMuted(std::format("running for {} s", m_uTicks / 10).c_str());
				Plugin::Api().EndSection();
			}
			DrawTour();
		}

	private:
		void DrawTour()
		{
			if (!Plugin::Api().BeginSection("api tour"))
				return;
			if (Plugin::Api().BeginTabBar("example_tour"))
			{
				if (Plugin::Api().BeginTabItem("memory", nullptr))
				{
					std::uintptr_t uBase = 0;
					std::size_t uSize = 0;
					if (Plugin::Mem::Module({}, uBase, uSize))
					{
						Plugin::Api().Text(std::format("main module at {:#x}, {} bytes", uBase, uSize).c_str());
						Plugin::Api().Text(std::format("dos header found at {:#x}", Plugin::Mem::Scan({}, "4D 5A")).c_str());
						if (const std::optional<std::uint16_t> optMagic = Plugin::Mem::Read<std::uint16_t>(uBase))
							Plugin::Api().Text(std::format("first word {:#06x}", *optMagic).c_str());
					}
					Plugin::Api().EndTabItem();
				}
				if (Plugin::Api().BeginTabItem("locale", nullptr))
				{
					Plugin::Api().Text(Plugin::Loc::Get("example_plugin.greeting").c_str());
					Plugin::Api().TextMuted(std::format("language {}", Plugin::Api().LocLocale ? Plugin::Api().LocLocale() : "unknown").c_str());
					Plugin::Api().EndTabItem();
				}
				if (Plugin::Api().BeginTabItem("input", nullptr))
				{
					Plugin::Api().Text(Plugin::Input::KeyDown(0x10) ? "shift is held" : "shift is not held");
					Plugin::Api().Text(Plugin::Input::MouseDown(0) ? "left mouse is held" : "left mouse is not held");
					Plugin::Api().EndTabItem();
				}
				if (Plugin::Api().BeginTabItem("table", nullptr))
				{
					if (Plugin::Api().BeginTable("example_table", 2, 0))
					{
						Plugin::Api().TableSetupColumn("name", 0, 0.f);
						Plugin::Api().TableSetupColumn("value", 0, 0.f);
						Plugin::Api().TableHeadersRow();
						Plugin::Api().TableNextRow();
						Plugin::Api().TableNextColumn();
						Plugin::Api().Text("frame");
						Plugin::Api().TableNextColumn();
						Plugin::Api().Text(std::format("{}", Plugin::Api().GetFrameCount()).c_str());
						Plugin::Api().TableNextRow();
						Plugin::Api().TableNextColumn();
						Plugin::Api().Text("delta");
						Plugin::Api().TableNextColumn();
						Plugin::Api().Text(std::format("{:.2f} ms", Plugin::Api().GetDeltaTime() * 1000.f).c_str());
						Plugin::Api().EndTable();
					}
					Plugin::Api().EndTabItem();
				}
				Plugin::Api().EndTabBar();
			}
			Plugin::Api().EndSection();
		}

		bool* m_pVisible = nullptr;
		float* m_pRadius = nullptr;
		int* m_pCorner = nullptr;
		std::uint32_t* m_pColor = nullptr;
		void* m_pKeybind = nullptr;
		unsigned int m_uTicks = 0;
	};

	clBannerFeature g_clBanner;

	bool Load()
	{
		Plugin::Log(Plugin::ELogLevel::Info, kBanner);
		return Plugin::RegisterFeature(g_clBanner);
	}

	void Unload()
	{
		Plugin::Log(Plugin::ELogLevel::Info, "goodbye from ExamplePlugin");
	}
}

OMEGAWARE_PLUGIN(Load, Unload)
