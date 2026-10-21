#pragma once

#include "Core/Log.hpp"
#include "Memory/Safety.hpp"

#include "OmegaRenderHook/Hook/HookScope.hpp"
#include "OmegaRenderHook/Hook/InlineHook.hpp"

#include <chrono>
#include <cstdint>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace OmegaWare::Hooks
{
	enum class EHookStatus : std::uint8_t
	{
		Uninstalled,
		Installed,
		Failed,
	};

	[[nodiscard]] constexpr std::string_view ToString(EHookStatus eStatus)
	{
		switch (eStatus)
		{
		case EHookStatus::Installed: return "installed";
		case EHookStatus::Failed: return "failed";
		default: return "uninstalled";
		}
	}

	using clCallGuard = OmegaRenderHook::clHookScopeOf<OmegaRenderHook::EHookDomain::Game>;

	inline bool WaitForCalls(std::chrono::milliseconds msTimeout)
	{
		return clCallGuard::WaitForDrain(msTimeout);
	}

	template <typename Fn>
		requires std::is_pointer_v<Fn> && std::is_function_v<std::remove_pointer_t<Fn>>
	class clHook
	{
	public:
		explicit clHook(std::string_view strName = "hook") : m_strName(strName) {}
		~clHook() { Remove(); }

		clHook(const clHook&) = delete;
		clHook& operator=(const clHook&) = delete;

		bool Install(void* pTarget, Fn pfnDetour)
		{
			if (m_eStatus == EHookStatus::Installed)
			{
				LogWarning("hook \"{}\" is already installed", m_strName);
				return false;
			}
			if (!pTarget || !Memory::IsExecutable(pTarget) || !pfnDetour)
			{
				LogError("hook \"{}\" target {} is not executable code", m_strName, pTarget);
				m_eStatus = EHookStatus::Failed;
				return false;
			}
			if (!m_clHook.Install(pTarget, reinterpret_cast<void*>(pfnDetour)))
			{
				LogError("hook \"{}\" failed to install at {}", m_strName, pTarget);
				m_eStatus = EHookStatus::Failed;
				return false;
			}
			m_pTarget = pTarget;
			m_eStatus = EHookStatus::Installed;
			LogDebug("hook \"{}\" installed at {}", m_strName, pTarget);
			return true;
		}

		bool Install(std::uintptr_t uTarget, Fn pfnDetour)
		{
			return Install(reinterpret_cast<void*>(uTarget), pfnDetour);
		}

		bool InstallVirtual(const void* pObject, std::size_t uIndex, Fn pfnDetour)
		{
			return Install(Memory::GetVirtualMethod(pObject, uIndex), pfnDetour);
		}

		bool Remove()
		{
			if (m_eStatus != EHookStatus::Installed)
				return true;
			const bool bDisabled = m_clHook.Disable();
			if (!WaitForCalls(std::chrono::milliseconds(500)))
				LogWarning("hook \"{}\" removed while calls were still in flight", m_strName);
			m_clHook.Uninstall();
			m_eStatus = EHookStatus::Uninstalled;
			LogDebug("hook \"{}\" removed from {}", m_strName, m_pTarget);
			return bDisabled;
		}

		template <typename... Args>
		decltype(auto) operator()(Args&&... args) const
		{
			const Fn pfnOriginal = m_eStatus == EHookStatus::Installed ? m_clHook.template Original<Fn>() : reinterpret_cast<Fn>(m_pTarget);
			return pfnOriginal(std::forward<Args>(args)...);
		}

		[[nodiscard]] EHookStatus GetStatus() const { return m_eStatus; }
		[[nodiscard]] bool IsInstalled() const { return m_eStatus == EHookStatus::Installed; }
		[[nodiscard]] void* GetTarget() const { return m_pTarget; }
		[[nodiscard]] std::string_view GetName() const { return m_strName; }

	private:
		OmegaRenderHook::clInlineHook m_clHook;
		void* m_pTarget = nullptr;
		EHookStatus m_eStatus = EHookStatus::Uninstalled;
		std::string m_strName;
	};
}
