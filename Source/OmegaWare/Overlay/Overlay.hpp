#pragma once

#include "OmegaRenderHook/OmegaRenderHook.hpp"
#include "OmegaGUI/Backends/DX10/BackendDX10.hpp"
#include "OmegaGUI/Backends/DX11/BackendDX11.hpp"
#include "OmegaGUI/Backends/DX12/BackendDX12.hpp"
#include "OmegaGUI/Backends/DX9/BackendDX9.hpp"
#include "OmegaGUI/Backends/OpenGL/BackendOpenGL.hpp"
#if OMEGAGUI_BACKEND_VULKAN && OMEGARENDERHOOK_VULKAN
#define OMEGAWARE_VULKAN 1
#include "OmegaGUI/Backends/Vulkan/BackendVulkan.hpp"
#endif

#include <atomic>
#include <cstddef>
#include <mutex>

namespace OmegaWare
{
	class clOverlay final : public OmegaRenderHook::clRenderListener
	{
	public:
		void OnAttach(const OmegaRenderHook::stDeviceDX12& rDevice) override;
		void OnFrame(const OmegaRenderHook::stFrameDX12& rFrame) override;
		void OnAttach(const OmegaRenderHook::stDeviceDX11& rDevice) override;
		void OnFrame(const OmegaRenderHook::stFrameDX11& rFrame) override;
		void OnAttach(const OmegaRenderHook::stDeviceDX10& rDevice) override;
		void OnFrame(const OmegaRenderHook::stFrameDX10& rFrame) override;
		void OnAttach(const OmegaRenderHook::stDeviceDX9& rDevice) override;
		void OnFrame(const OmegaRenderHook::stFrameDX9& rFrame) override;
		void OnAttach(const OmegaRenderHook::stDeviceOpenGL& rDevice) override;
		void OnFrame(const OmegaRenderHook::stFrameOpenGL& rFrame) override;
#if OMEGAWARE_VULKAN
		void OnAttach(const OmegaRenderHook::stDeviceVulkan& rDevice) override;
		void OnFrame(const OmegaRenderHook::stFrameVulkan& rFrame) override;
#endif
		void OnDetach(OmegaRenderHook::ERenderApi eApi, OmegaRenderHook::EDetachReason eReason) override;
		bool OnWindowMessage(HWND hWindow, UINT uMessage, WPARAM uWParam, LPARAM iLParam, LRESULT& rResult) override;

		void Shutdown();
		[[nodiscard]] bool IsAttached() const { return m_bAttached; }
		[[nodiscard]] std::unique_lock<std::recursive_mutex> LockGui() { return std::unique_lock(m_mtxGui); }

	private:
		void AttachGui(OmegaGUI::clRenderBackend* pBackend, HWND hWindow, UINT uHeight);
		void DetachGui();
		void DrawFrame();

		std::recursive_mutex m_mtxGui;
		OmegaGUI::clBackendDX11 m_clBackendDX11;
		OmegaGUI::clBackendDX12 m_clBackendDX12;
		OmegaGUI::clBackendDX10 m_clBackendDX10;
		OmegaGUI::clBackendDX9 m_clBackendDX9;
		OmegaGUI::clBackendOpenGL m_clBackendOpenGL;
#if OMEGAWARE_VULKAN
		OmegaGUI::clBackendVulkan m_clBackendVulkan;
#endif
		OmegaGUI::clRenderBackend* m_pActiveBackend = nullptr;
		OmegaGUI::stWorldPass m_stWorldPass;
		bool m_bGuiInitialized = false;
		std::size_t m_uLocaleSubscription = 0;
		float m_flScale = 1.f;
		bool m_bDisplayKnown = false;
		std::atomic<bool> m_bAttached = false;
	};
}
