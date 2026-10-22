#include "Overlay/Overlay.hpp"
#include "Core/Framework.hpp"
#include "Config/Config.hpp"
#include "Core/Console.hpp"
#include "Core/Log.hpp"
#include "Features/Feature.hpp"
#include "Locales/Locales.hpp"
#include "Menu/Menu.hpp"
#include "Plugins/Plugins.hpp"

#include "OmegaGUI/OmegaGUI.hpp"
#include "OmegaLocalization/GUI.hpp"

namespace OmegaWare
{
	void clOverlay::OnAttach(const OmegaRenderHook::stDeviceDX12& rDevice)
	{
		std::scoped_lock lckGui(m_mtxGui);
		OmegaGUI::stDX12Config stConfig;
		stConfig.pDevice = rDevice.pDevice;
		stConfig.pQueue = rDevice.pQueue;
		stConfig.iFramesInFlight = static_cast<int>(rDevice.uFramesInFlight);
		stConfig.eRenderTargetFormat = rDevice.eFormat;
		if (!m_clBackendDX12.Initialize(stConfig))
		{
			OmegaRenderHook::Log(OmegaRenderHook::ELogLevel::Error, "gui Direct3D 12 backend failed to initialize");
			return;
		}
		AttachGui(&m_clBackendDX12, rDevice.hWindow, rDevice.uHeight);
	}

	void clOverlay::OnFrame(const OmegaRenderHook::stFrameDX12& rFrame)
	{
		std::scoped_lock lckGui(m_mtxGui);
		if (m_pActiveBackend != &m_clBackendDX12)
			return;
		m_clBackendDX12.BeginFrame(rFrame.pCommandList);
		m_clBackendDX12.SetWorldTarget(rFrame.hRenderTarget, rFrame.hDepthStencil, rFrame.eDepthFormat, rFrame.pDevice->uWidth, rFrame.pDevice->uHeight);
		m_stWorldPass = {};
		m_stWorldPass.bReversedZ = rFrame.bReversedZ;
		DrawFrame();
	}

	void clOverlay::OnAttach(const OmegaRenderHook::stDeviceDX11& rDevice)
	{
		std::scoped_lock lckGui(m_mtxGui);
		if (!m_clBackendDX11.Initialize(rDevice.pDevice, rDevice.pContext))
		{
			OmegaRenderHook::Log(OmegaRenderHook::ELogLevel::Error, "gui Direct3D 11 backend failed to initialize");
			return;
		}
		AttachGui(&m_clBackendDX11, rDevice.hWindow, rDevice.uHeight);
	}

	void clOverlay::OnFrame(const OmegaRenderHook::stFrameDX11& rFrame)
	{
		std::scoped_lock lckGui(m_mtxGui);
		if (m_pActiveBackend != &m_clBackendDX11)
			return;
		m_clBackendDX11.SetWorldTarget(rFrame.pRenderTarget, rFrame.pDepthStencil);
		m_stWorldPass.bReversedZ = rFrame.bReversedZ;
		DrawFrame();
		m_clBackendDX11.SetWorldTarget(nullptr, nullptr);
	}

	void clOverlay::OnAttach(const OmegaRenderHook::stDeviceDX10& rDevice)
	{
		std::scoped_lock lckGui(m_mtxGui);
		if (!m_clBackendDX10.Initialize(rDevice.pDevice))
		{
			OmegaRenderHook::Log(OmegaRenderHook::ELogLevel::Error, "gui Direct3D 10 backend failed to initialize");
			return;
		}
		AttachGui(&m_clBackendDX10, rDevice.hWindow, rDevice.uHeight);
	}

	void clOverlay::OnFrame(const OmegaRenderHook::stFrameDX10&)
	{
		std::scoped_lock lckGui(m_mtxGui);
		if (m_pActiveBackend != &m_clBackendDX10)
			return;
		m_stWorldPass = {};
		DrawFrame();
	}

	void clOverlay::OnAttach(const OmegaRenderHook::stDeviceDX9& rDevice)
	{
		std::scoped_lock lckGui(m_mtxGui);
		if (!m_clBackendDX9.Initialize(rDevice.pDevice))
		{
			OmegaRenderHook::Log(OmegaRenderHook::ELogLevel::Error, "gui Direct3D 9 backend failed to initialize");
			return;
		}
		AttachGui(&m_clBackendDX9, rDevice.hWindow, rDevice.uHeight);
	}

	void clOverlay::OnFrame(const OmegaRenderHook::stFrameDX9& rFrame)
	{
		std::scoped_lock lckGui(m_mtxGui);
		if (m_pActiveBackend != &m_clBackendDX9)
			return;
		m_clBackendDX9.SetWorldDepthSurface(rFrame.pDepthStencil);
		m_stWorldPass = {};
		DrawFrame();
		m_clBackendDX9.SetWorldDepthSurface(nullptr);
	}

	void clOverlay::OnAttach(const OmegaRenderHook::stDeviceOpenGL& rDevice)
	{
		std::scoped_lock lckGui(m_mtxGui);
		if (!m_clBackendOpenGL.Initialize())
		{
			OmegaRenderHook::Log(OmegaRenderHook::ELogLevel::Error, "gui OpenGL backend failed to initialize");
			return;
		}
		AttachGui(&m_clBackendOpenGL, rDevice.hWindow, rDevice.uHeight);
	}

	void clOverlay::OnFrame(const OmegaRenderHook::stFrameOpenGL&)
	{
		std::scoped_lock lckGui(m_mtxGui);
		if (m_pActiveBackend != &m_clBackendOpenGL)
			return;
		m_stWorldPass = {};
		DrawFrame();
	}

#if OMEGAWARE_VULKAN
	void clOverlay::OnAttach(const OmegaRenderHook::stDeviceVulkan& rDevice)
	{
		std::scoped_lock lckGui(m_mtxGui);
		OmegaGUI::stVulkanConfig stConfig;
		stConfig.pfnGetInstanceProcAddr = rDevice.pfnGetInstanceProcAddr;
		stConfig.hInstance = rDevice.hInstance;
		stConfig.hPhysicalDevice = rDevice.hPhysicalDevice;
		stConfig.hDevice = rDevice.hDevice;
		stConfig.uQueueFamily = rDevice.uQueueFamily;
		stConfig.hQueue = rDevice.hQueue;
		stConfig.hRenderPass = rDevice.hRenderPass;
		stConfig.eTargetFormat = rDevice.eFormat;
		stConfig.bWorldHasDepth = true;
		stConfig.uFramesInFlight = rDevice.uImageCount;
		if (!m_clBackendVulkan.Initialize(stConfig))
		{
			OmegaRenderHook::Log(OmegaRenderHook::ELogLevel::Error, "gui Vulkan backend failed to initialize");
			return;
		}
		AttachGui(&m_clBackendVulkan, rDevice.hWindow, rDevice.uHeight);
	}

	void clOverlay::OnFrame(const OmegaRenderHook::stFrameVulkan& rFrame)
	{
		std::scoped_lock lckGui(m_mtxGui);
		if (m_pActiveBackend != &m_clBackendVulkan)
			return;
		m_clBackendVulkan.BeginFrame(rFrame.hCommandBuffer, rFrame.stExtent);
		m_stWorldPass = {};
		m_stWorldPass.bReversedZ = rFrame.bReversedZ;
		DrawFrame();
	}
#endif

	void clOverlay::OnDetach(OmegaRenderHook::ERenderApi eApi, OmegaRenderHook::EDetachReason eReason)
	{
		std::scoped_lock lckGui(m_mtxGui);
		OmegaRenderHook::Log(OmegaRenderHook::ELogLevel::Info, "gui detached from {} ({})", OmegaRenderHook::ToString(eApi), OmegaRenderHook::ToString(eReason));
		DetachGui();
	}

	bool clOverlay::OnWindowMessage(HWND hWindow, UINT uMessage, WPARAM uWParam, LPARAM iLParam, LRESULT& rResult)
	{
		std::scoped_lock lckGui(m_mtxGui);
		if (!m_bGuiInitialized || !OmegaGUI::ProcessWin32Message(hWindow, uMessage, uWParam, iLParam))
			return false;
		rResult = uMessage == WM_SETCURSOR ? TRUE : 0;
		return true;
	}

	void clOverlay::Shutdown()
	{
		std::scoped_lock lckGui(m_mtxGui);
		DetachGui();
		if (!m_bGuiInitialized)
			return;
		OmegaLocalization::Unsubscribe(m_uLocaleSubscription);
		OmegaGUI::Shutdown();
		m_bGuiInitialized = false;
	}

	void clOverlay::AttachGui(OmegaGUI::clRenderBackend* pBackend, HWND hWindow, UINT uHeight)
	{
		m_pActiveBackend = pBackend;
		Plugins::SetImageBackend(pBackend);
		m_bDisplayKnown = false;
		if (m_bGuiInitialized)
		{
			OmegaGUI::SetBackend(pBackend);
			m_bAttached = true;
			OmegaRenderHook::Log(OmegaRenderHook::ELogLevel::Info, "gui reattached to {}", pBackend->GetName());
			return;
		}

		OmegaGUI::stConfig stConfig;
		stConfig.pathSettings = DataDirectory() / "Layout.ini";
		m_flScale = Menu::ResolveScale(static_cast<float>(uHeight));
		stConfig.flScale = m_flScale;
		if (!OmegaGUI::Initialize(pBackend, hWindow, stConfig))
		{
			OmegaRenderHook::Log(OmegaRenderHook::ELogLevel::Error, "gui failed to initialize: {}", OmegaGUI::GetInitError());
			DetachGui();
			return;
		}

		std::string strErrors;
		m_uLocaleSubscription = OmegaLocalization::GUI::Attach({ g_stLocaleEnglish, g_stLocaleGerman, g_stLocalePolish }, "en", DataDirectory() / "Locales", &strErrors);
		if (!strErrors.empty())
			OmegaRenderHook::Log(OmegaRenderHook::ELogLevel::Warning, "locale overrides: {}", strErrors);
		m_bGuiInitialized = true;
		m_bAttached = true;
		OmegaRenderHook::Log(OmegaRenderHook::ELogLevel::Info, "gui initialized on {} in {} at {:.2f}x scale", pBackend->GetName(), OmegaLocalization::GetLocale(), m_flScale);
	}

	void clOverlay::DetachGui()
	{
		m_bAttached = false;
		OmegaRenderHook::InputBlock::SetBlocking({});
		Plugins::SetImageBackend(nullptr);
		if (m_bGuiInitialized)
			OmegaGUI::SetBackend(nullptr);
		if (m_pActiveBackend == &m_clBackendDX12)
			m_clBackendDX12.Shutdown();
		else if (m_pActiveBackend == &m_clBackendDX11)
			m_clBackendDX11.Shutdown();
		else if (m_pActiveBackend == &m_clBackendDX10)
			m_clBackendDX10.Shutdown();
		else if (m_pActiveBackend == &m_clBackendDX9)
			m_clBackendDX9.Shutdown();
		else if (m_pActiveBackend == &m_clBackendOpenGL)
			m_clBackendOpenGL.Shutdown();
#if OMEGAWARE_VULKAN
		else if (m_pActiveBackend == &m_clBackendVulkan)
			m_clBackendVulkan.Shutdown();
#endif
		m_pActiveBackend = nullptr;
	}

	void clOverlay::DrawFrame()
	{
		const float flHeight = OmegaGUI::GetDisplaySize().y;
		if (const float flScale = Menu::ResolveScale(flHeight); m_bDisplayKnown && flHeight > 0.f && flScale != m_flScale)
		{
			m_flScale = flScale;
			OmegaGUI::SetScale(flScale);
			OmegaRenderHook::Log(OmegaRenderHook::ELogLevel::Info, "gui scale changed to {:.2f}x", flScale);
		}
		OmegaGUI::NewFrame();
		m_bDisplayKnown = true;
		const bool bBlockAll = OmegaGUI::IsVisible() && OmegaGUI::GetConfig().bBlockInputWhenVisible;
		OmegaRenderHook::InputBlock::SetBlocking({ bBlockAll || OmegaGUI::WantsKeyboard(), bBlockAll || OmegaGUI::WantsMouse(), OmegaGUI::WantsGamepad() });
		Config::UpdateKeybinds();
		Plugins::FlushImages();
		const Menu::stHotkeys stKeys = Menu::Hotkeys();
		if (stKeys.iMenu > 0)
			OmegaGUI::GetConfig().uToggleKey = static_cast<OmegaGUI::KeyCode>(stKeys.iMenu);
		if (!OmegaGUI::WantsKeyboard())
		{
			const OmegaGUI::clInput& rInput = OmegaGUI::GetInput();
			if (stKeys.iUnload > 0 && rInput.IsKeyPressed(static_cast<OmegaGUI::KeyCode>(stKeys.iUnload)))
				RequestUnload();
			if (stKeys.iConsole > 0 && rInput.IsKeyPressed(static_cast<OmegaGUI::KeyCode>(stKeys.iConsole)))
				Menu::ToggleConsole();
		}
		if (!IsUnloadRequested())
			Features::RenderAll();
		Menu::Draw(m_pActiveBackend ? m_pActiveBackend->GetName() : std::string_view{});
		OmegaGUI::EndFrame();
		OmegaGUI::RenderWorld(m_stWorldPass);
		OmegaGUI::Render();
	}
}
