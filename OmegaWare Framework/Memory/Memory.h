#pragma once

#include "pch.h"
#include "WindowsTypes.h"

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

	// Gets loaded module handle
	HMODULE GetModule(std::string sModuleName);

	// Type used for EnumerateHandles function
	typedef std::function<bool(PSYSTEM_HANDLE_TABLE_ENTRY_INFO)>EnumerateHandlesFunc;
	// Enumerates open handles with fn that returns true if it wants to stop the enumeration
	void EnumerateHandles(EnumerateHandlesFunc fn);

	// Get a handle to current process with PROCESS_QUERY_INFORMATION and PROCESS_VM_READ
	HANDLE GetPrivilegedHandleToProcess(DWORD dwProcessId = 0);

	// Type used for EnumerateModules function
	typedef std::function<bool(std::string)>EnumerateModulesFunc;
	// Enum used for EnumerateModules function, filters the strings provided to the callback fn
	namespace EnumerateModulesFlags {
		constexpr DWORD DiscardSystemModules = 1 << 0; // No SYSTEM32 Modules
		constexpr DWORD ModuleNameOnly       = 1 << 1; // Instead of full path
		constexpr DWORD LowercaseName        = 1 << 2; // Passes a fully lowercase name
	}
	// Enumerates process moduleswith fn that returns true if it wants to stop the enumeration
	void EnumerateModules(EnumerateModulesFunc fn, DWORD dwProcessId = 0, DWORD flags = 0);

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

		// Call the original function that we hooked
		template <typename... TArgs>
		auto operator()(TArgs... args) 
		{
			static_assert(std::is_invocable_v<TFunction, TArgs...>, "Args dont match function type!");
			if (!m_fnOriginal)
				throw std::runtime_error("Attempted to call a nullptr!");
			
			return m_fnOriginal(std::forward<TArgs>(args)...);
		};
		
		// Disable the already installed hook (wont remove it)
		MH_STATUS Disable() 
		{
			return MH_DisableHook(m_pTarget);
		};

		// Enable the already installed hook
		MH_STATUS Enable() 
		{
			return MH_EnableHook(m_pTarget);
		};

		// Remove the installed hook
		MH_STATUS RemoveHook() 
		{
			MH_STATUS status = Disable();

			if (status != MH_OK && status != MH_ERROR_DISABLED)
				return status;

			status = MH_RemoveHook(m_pTarget);
			if (status == MH_OK)
				m_pTarget = nullptr;

			return status;
		};

		// Hook function via a function pointer
		MH_STATUS HookAddress(TFunction fnHook, void* pTarget)
		{
			if (MH_STATUS status = MH_CreateHook(pTarget, fnHook, reinterpret_cast<void**>(&m_fnOriginal)); status != MH_OK)
				return status;

			m_pTarget = pTarget;
			return Enable();
		};

		// Hook the function via a virtual method table index
		MH_STATUS HookVirtualMethod(TFunction fnHook, void* lpAddress, size_t index)
		{
			return HookAddress(fnHook, GetVirtualMethod(lpAddress, index));
		}
	};
}