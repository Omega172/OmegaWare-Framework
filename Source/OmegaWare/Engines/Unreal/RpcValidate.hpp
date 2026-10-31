#pragma once

#if OMEGAWARE_ENGINE_UNREAL

#include "Engines/Unreal/Unreal.hpp"
#include "Hooks/Hook.hpp"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

namespace OmegaWare::Unreal
{
	enum class EValidateStatus : std::uint8_t
	{
		Unresolved,
		Ready,
		Failed,
		Faulted,
	};

	[[nodiscard]] std::string_view ToString(EValidateStatus eStatus);
	[[nodiscard]] bool IsNetHost();
	[[nodiscard]] std::optional<std::size_t> FindValidateOffset(SDK::UFunction* pFunction);

	class clValidateOverrideBase
	{
	public:
		clValidateOverrideBase(const clValidateOverrideBase&) = delete;
		clValidateOverrideBase& operator=(const clValidateOverrideBase&) = delete;

		bool Attach(const SDK::UObject* pObject);
		void DetachAll();

		[[nodiscard]] std::string_view GetFunctionName() const { return m_strFunction; }
		[[nodiscard]] EValidateStatus GetStatus() const { return m_eStatus.load(std::memory_order_acquire); }
		[[nodiscard]] std::uint64_t GetOverrides() const { return m_uOverrides.load(std::memory_order_relaxed); }

	protected:
		clValidateOverrideBase(std::string_view strFunction, void* pDetour);
		~clValidateOverrideBase() { DetachAll(); }

		[[nodiscard]] void* OriginalFor(const SDK::UObject* pObject) const;
		[[nodiscard]] bool RunPolicy(const std::function<bool()>& fnPolicy);

	private:
		bool Resolve();

		std::string m_strFunction;
		void* m_pDetour = nullptr;
		SDK::UFunction* m_pFunction = nullptr;
		std::size_t m_uOffset = 0;
		std::atomic<EValidateStatus> m_eStatus = EValidateStatus::Unresolved;
		std::atomic<std::uint64_t> m_uOverrides = 0;
		mutable std::shared_mutex m_mtxPatched;
		std::unordered_map<void**, void*> m_mapPatched;
		std::unordered_set<void**> m_setTrivial;
	};

	template <typename Tag, typename... Args>
	class clValidateOverride final : public clValidateOverrideBase
	{
	public:
		using FnPolicy = std::function<bool(SDK::UObject*, Args...)>;

		clValidateOverride(std::string_view strFunction, FnPolicy fnPolicy) : clValidateOverrideBase(strFunction, reinterpret_cast<void*>(&Detour)), m_fnPolicy(std::move(fnPolicy))
		{
			s_pInstance = this;
		}

	private:
		using FnValidate = bool (*)(SDK::UObject*, Args...);

		static bool Detour(SDK::UObject* pObject, Args... args)
		{
			Hooks::clCallGuard clGuard;
			clValidateOverride* pSelf = s_pInstance;
			const auto pfnOriginal = reinterpret_cast<FnValidate>(pSelf->OriginalFor(pObject));
			if (!pfnOriginal)
				return false;
			if (pfnOriginal(pObject, args...))
				return true;
			if (!IsNetHost())
				return false;
			return pSelf->RunPolicy([&] { return pSelf->m_fnPolicy(pObject, args...); });
		}

		static inline clValidateOverride* s_pInstance = nullptr;
		FnPolicy m_fnPolicy;
	};
}

#endif
