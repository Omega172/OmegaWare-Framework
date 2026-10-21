#pragma once

#include "Hooks/Hook.hpp"

#include <functional>
#include <utility>
#include <vector>

namespace OmegaWare::Hooks
{
	class clHookBatch
	{
	public:
		clHookBatch() = default;
		~clHookBatch()
		{
			if (!m_bCommitted)
				Rollback();
		}

		clHookBatch(const clHookBatch&) = delete;
		clHookBatch& operator=(const clHookBatch&) = delete;

		template <typename Fn, typename... Args>
		bool Install(clHook<Fn>& rHook, Args&&... args)
		{
			if (!rHook.Install(std::forward<Args>(args)...))
			{
				m_bFailed = true;
				return false;
			}
			m_aRollbacks.emplace_back([&rHook] { rHook.Remove(); });
			return true;
		}

		template <typename Fn, typename... Args>
		bool InstallVirtual(clHook<Fn>& rHook, Args&&... args)
		{
			if (!rHook.InstallVirtual(std::forward<Args>(args)...))
			{
				m_bFailed = true;
				return false;
			}
			m_aRollbacks.emplace_back([&rHook] { rHook.Remove(); });
			return true;
		}

		bool Commit()
		{
			if (m_bFailed)
			{
				Rollback();
				return false;
			}
			m_bCommitted = true;
			return true;
		}

		void Rollback()
		{
			for (auto itRollback = m_aRollbacks.rbegin(); itRollback != m_aRollbacks.rend(); ++itRollback)
				(*itRollback)();
			m_aRollbacks.clear();
		}

		[[nodiscard]] bool HasFailed() const { return m_bFailed; }

	private:
		std::vector<std::function<void()>> m_aRollbacks;
		bool m_bCommitted = false;
		bool m_bFailed = false;
	};
}
