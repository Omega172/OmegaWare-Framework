#pragma once

#include <atomic>
#include <span>
#include <string>
#include <string_view>

namespace OmegaWare
{
	class clFeature
	{
	public:
		clFeature();
		virtual ~clFeature() = default;

		clFeature(const clFeature&) = delete;
		clFeature& operator=(const clFeature&) = delete;

		[[nodiscard]] virtual std::string_view GetName() const = 0;
		[[nodiscard]] virtual std::string GetDisplayName() const;

		virtual bool Setup() { return true; }
		virtual void Destroy() {}
		virtual void Run() {}
		virtual void Render() {}
		virtual void DrawMenu() {}

		[[nodiscard]] bool IsActive() const { return m_bActive; }
		[[nodiscard]] bool IsFaulted() const { return m_bFaulted; }

	private:
		friend struct stFeatureAccess;

		std::atomic<bool> m_bActive = false;
		std::atomic<bool> m_bFaulted = false;
		bool m_bSetup = false;
	};

	namespace Features
	{
		[[nodiscard]] std::span<clFeature* const> All();
		void SetupAll();
		void Setup(clFeature& rFeature);
		void Remove(clFeature& rFeature);
		void RunAll();
		void RenderAll();
		[[nodiscard]] std::string DisplayName(const clFeature& rFeature);
		void DrawPage(clFeature& rFeature);
		void DestroyAll();
	}
}
