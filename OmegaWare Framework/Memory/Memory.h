#pragma once

#include "pch.h"

#define IsValidObjectPtr(lpAddress) (Memory::IsReadable(lpAddress, sizeof(*lpAddress)))
#define IsValidPtr(lpAddress) (Memory::IsReadable(lpAddress, sizeof(void*)))

namespace Memory
{
	// Checks to see if memory is readable to prevent access violations, can be expensive
	bool IsReadable(const void* lpAddress, size_t dwLength);

	// Gets the length of a string while doing readable checks, more expensive than std::strlen
	size_t Strlen(const char* lpAddress, size_t dwMaxSize = 1024);
	// Gets the length of a wstring while doing readable checks, more expensive than std::wcslen
	size_t Wcslen(const wchar_t* lpAddress, size_t dwMaxSize = 1024);

	// Gets method of type T from virtual method table
	void* GetVirtualMethod(void* lpAddress, size_t index);

	template<typename TFunction>
	class Hook
	{
	private:
		void* m_pTarget = nullptr;
		TFunction m_fnOriginal = nullptr;

	public:
		Hook() {};

		template <typename... TArgs>
		auto operator()(TArgs... args) {
			static_assert(std::is_invocable_v<TFunction, TArgs...>, "Args dont match function type!");
			if (!m_fnOriginal)
				throw std::runtime_error("Attempted to call a nullptr!");
			
			return m_fnOriginal(std::forward<TArgs>(args)...);
		};



		MH_STATUS Disable() {
			return MH_DisableHook(m_pTarget);
		};

		MH_STATUS Enable() {
			return MH_EnableHook(m_pTarget);
		};

		MH_STATUS RemoveHook() {
			if (MH_STATUS status = Disable(); status != MH_OK)
				return status;

			return MH_RemoveHook(m_pTarget);
		};



		MH_STATUS HookAddress(TFunction fnHook, void* pTarget)
		{
			m_pTarget = pTarget;
			if (MH_STATUS status = MH_CreateHook(m_pTarget, fnHook, reinterpret_cast<void**>(&m_fnOriginal)); status != MH_OK)
				return status;

			return Enable();
		};

		MH_STATUS HookVirtualMethod(TFunction fnHook, void* lpAddress, size_t index)
		{
			return HookAddress(fnHook, GetVirtualMethod(lpAddress, index));
		}
	};
}