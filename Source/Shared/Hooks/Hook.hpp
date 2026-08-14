#pragma once

#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include <polyhook2/Detour/x64Detour.hpp>
#include <polyhook2/Misc.hpp>

#include "HookStatus.hpp"
#include "../Memory/PointerSafety.hpp"
#include "../Utils/Logging/Logging.hpp"

namespace Hooking
{
	/**
	 * Owns a single detour. PolyHook2 is an implementation detail behind this type; callers
	 * only ever see Start/Remove/GetStatus and the callable original-function syntax.
	 *
	 * A Hook<T> can never be left half-installed: Start() either fully succeeds (status
	 * becomes Installed) or fully fails (status becomes Failed, no PolyHook2 state retained).
	 * The destructor removes the hook automatically so a Hook<T> can't outlive its install.
	 */
	template<typename Fn_t>
	class Hook
	{
	private:
		std::unique_ptr<PLH::x64Detour> m_pDetour;
		uint64_t m_uTrampoline = 0;
		void* m_pTarget = nullptr;
		HookStatus m_eStatus = HookStatus::Uninstalled;

	public:
		Hook() = default;
		~Hook() { Remove(); }

		Hook(const Hook&) = delete;
		Hook& operator=(const Hook&) = delete;

		/**
		 * Call the original function. Only valid while the hook is Installed.
		 */
		template<typename... TArgs>
		auto operator()(TArgs&&... args) const
		{
			static_assert(std::is_invocable_v<Fn_t, TArgs...>, "Args dont match function type!");

			if (m_eStatus != HookStatus::Installed)
				throw std::runtime_error("Attempted to call the original function of a hook that is not installed.");

			return PLH::FnCast(m_uTrampoline, Fn_t{})(std::forward<TArgs>(args)...);
		};

		HookStatus GetStatus() const { return m_eStatus; };
		void* GetTarget() const { return m_pTarget; };

		/**
		 * Hook an address. Fails cleanly (no partial state) if the target is invalid, or if
		 * this Hook is already installed.
		 */
		bool Start(Fn_t fnDetour, void* lpAddress)
		{
			if (m_eStatus == HookStatus::Installed)
			{
				Utils::LogHook("Hook", "Failure", "Start() called on a hook that is already installed.");
				return false;
			}

			if (!lpAddress || !Memory::IsValidPtr(lpAddress))
			{
				Utils::LogHook("Hook", "Failure", "Target address is null or unreadable.");
				m_eStatus = HookStatus::Failed;
				return false;
			}

			auto pDetour = std::make_unique<PLH::x64Detour>(
				reinterpret_cast<uint64_t>(lpAddress),
				reinterpret_cast<uint64_t>(fnDetour),
				&m_uTrampoline);

			if (!pDetour->hook())
			{
				Utils::LogHook("Hook", "Failure", "PolyHook2 x64Detour::hook() failed.");
				m_uTrampoline = 0;
				m_eStatus = HookStatus::Failed;
				return false;
			}

			m_pDetour = std::move(pDetour);
			m_pTarget = lpAddress;
			m_eStatus = HookStatus::Installed;
			return true;
		};

		/**
		 * Hook a virtual method table slot.
		 */
		bool Start(Fn_t fnDetour, void* lpVTable, size_t iIndex)
		{
			return Start(fnDetour, Memory::GetVirtualMethod(lpVTable, iIndex));
		};

		/**
		 * Remove an installed hook. Safe to call on a hook that isn't installed (logged, not an error).
		 */
		bool Remove()
		{
			if (m_eStatus != HookStatus::Installed)
				return true;

			const bool bOk = m_pDetour->unHook();
			if (!bOk)
				Utils::LogHook("Hook", "Failure", "PolyHook2 x64Detour::unHook() failed.");

			m_pDetour.reset();
			m_uTrampoline = 0;
			m_pTarget = nullptr;
			m_eStatus = HookStatus::Uninstalled;

			return bOk;
		};
	};
}
