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
		}

	private:
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
