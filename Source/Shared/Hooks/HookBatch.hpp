#pragma once

#include <functional>
#include <utility>
#include <vector>

#include "Hook.hpp"

namespace Hooking
{
	/**
	 * Groups several Hook<T>::Start calls into a single transaction: validate/install each
	 * hook, and if any of them fails, every hook already installed in this batch is rolled
	 * back before Setup() returns. Call Commit() once every hook in the batch installed
	 * successfully; an uncommitted batch rolls itself back on destruction.
	 */
	class HookBatch
	{
	private:
		std::vector<std::function<void()>> m_vecRollbacks;
		bool m_bCommitted = false;

	public:
		~HookBatch()
		{
			if (!m_bCommitted)
				Rollback();
		};

		template<typename Fn_t, typename... TArgs>
		bool Install(Hook<Fn_t>& hook, TArgs&&... args)
		{
			if (!hook.Start(std::forward<TArgs>(args)...))
				return false;

			m_vecRollbacks.emplace_back([&hook]() { hook.Remove(); });
			return true;
		};

		void Commit() { m_bCommitted = true; };

		void Rollback()
		{
			for (auto itr = m_vecRollbacks.rbegin(); itr != m_vecRollbacks.rend(); ++itr)
				(*itr)();

			m_vecRollbacks.clear();
		};
	};
}
