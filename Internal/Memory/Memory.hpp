#pragma once
#include "pch.h"

#include "Windows/WindowsMemory.hpp"

#define IsValidObjectPtr(lpAddress) (Memory::IsReadable(lpAddress, sizeof(*lpAddress)))
#define IsValidPtr(lpAddress) (Memory::IsReadable(lpAddress, sizeof(void*)))

#define Signature(n) (ConvertSignatureArrayToVector(Memory::SignatureData_t::Conversion_t::build<stb::fixed_string{n}>::value))

namespace Memory
{
	/**
	 * Determine if the memory being pointed to is readable to prevent access violations.
	 * 
	 * \param lpAddress The address that is being checked.
	 * 
	 * \param dwLength The size of the object that you want to read from.
	 * 
	 * \warning Can be expensive!
	 */
	bool IsReadable(const void* lpAddress, size_t dwLength);

	/**
	 * Get the length of a string whilst using Memory::IsReadable to prevent access violations.
	 * 
	 * \returns The size of the string.
	 * 
	 * \param lpAddress The address of the start of the string to be checked.
	 * 
	 * \param dwMaxSize The maximum size of the string to be read from.
	 * 
	 * \warning Is more expensive than std::strlen!
	 */
	size_t Strlen(const char* lpAddress, size_t dwMaxSize = 1024);

	/**
	 * Get the length of a wstring whilst using Memory::IsReadable to prevent access violations.
	 *
	 * \returns The size of the wstring.
	 *
	 * \param lpAddress The address of the start of the wstring to be checked.
	 *
	 * \param dwMaxSize The maximum size of the wstring to be read from.
	 *
	 * \warning Is more expensive than std::wcslen!
	 */
	size_t Wcslen(const wchar_t* lpAddress, size_t dwMaxSize = 1024);

	struct InterfaceRegistry_t {
		void* (cdecl* m_fnCreate)();
		const char* m_szName;
		InterfaceRegistry_t* m_pNext;
	};
	
	/**
	 * Enumerate interfaces in module; calling the function and passing a pointer to the interface registry.
	 * 
	 * \param svModuleName The name of the module to look for interfaces in.
	 * 
	 * \param fn A function that accepts an InterfaceRegistry pointer and returns a boolean indicating if it wants to stop the enumeration.
	 */
	void EnumerateInterfaces(std::string_view svModuleName, std::function<bool(InterfaceRegistry_t*)> fn);

	/**
	 * Find calls the Create function for the given interface found in the given module.
	 *
	 * \returns A pointer to the interface.
	 * 
	 * \param svModuleName The name of the module to look for the interface in.
	 * 
	 * \param svInterfaceName The full name of the interface to find in the module.
	 */
	void* CreateInterface(std::string_view svModuleName, std::string_view svInterfaceName);

	/**
	 * \returns The number of methods in the virtual method table.
	 * 
	 * \param lpAddress The address to the virtual method table.
	 */
	size_t GetVirtualMethodTableSize(void* lpAddress);

	/**
	 * \returns A pointer to the method at the given index in the virtual method table.
	 * 
	 * \param lpAddress The address to the virtual method table.
	 * 
	 * \param index The index of the method to get the offset of in the virtual method table.
	 */
	void* GetVirtualMethod(void* lpAddress, size_t index);

	struct SignatureData_t {
		using Span_t = std::vector<int16_t>;
		using Conversion_t = stb::basic_hex_string_array_conversion<' ', '?', Span_t::value_type, -1>;

		Span_t aSignature;
		std::function<uintptr_t (uintptr_t)> CorrectReturnAddressFunc;
	};
 
	/**
	 * Scan a module for the given signature.
	 *
	 * \returns A pointer to the memory address of the start of the signature.
	 *
	 * \param svModuleName The name of the module to look for the signature in.
	 *
	 * \param aSignatures The signature to scan the module with.
	 */
	void* SignatureScan(const std::string_view svModuleName, const SignatureData_t::Span_t aSignature);
	
	/**
	 * Scan a module for the given signature.
	 *
	 * \returns A pointer to the memory address of the start of the signature.
	 *
	 * \param hModule The module to look for the signature in.
	 *
	 * \param aSignatures The signature to scan the module with.
	 */
	void* SignatureScan(const HMODULE hModule, const SignatureData_t::Span_t aSignature);

	/**
	 * Scan a module for one of the given signatures.
	 * 
	 * \returns A pointer to the first memory address of the start of the first successful signature.
	 * 
	 * \param svModuleName The name of the module to look for the signatures in.
	 * 
	 * \param vecSignatures A vector of signatures to scan the module with.
	 */
	void* SignatureScan(const std::string_view svModuleName, const std::vector<SignatureData_t> vecSignatures);

	/**
	 * Scan a module for one of the given signatures.
	 *
	 * \returns A pointer to the first memory address of the start of the first successful signature.
	 *
	 * \param hModule The module to look for the signatures in.
	 *
	 * \param vecSignatures A vector of signatures to scan the module with.
	 */
	void* SignatureScan(const HMODULE hModule, const std::vector<SignatureData_t> vecSignatures);

	struct ModuleScanResult_t {
		std::string m_sModuleName;
		std::vector<void*> m_vecPointers;
	};

	/**
	 * Scan the current process and its modules for all instances of the given signatures.
	 *
	 * \returns A vector of ModuleScanResult_t that give vectors of pointers to instructions and the module it belongs to.
	 * 
	 * \vecModules The modules to scan.
	 * 
	 * \bIncludeProcess Wether or not to scan the target process.
	 * 
	 * \param vecSignatures A vector of SignatureData_t to scan all modules with.
	 *
	 * \param sizePerModuleLimit The limit of instructions to collect per module.
	 */
	std::vector<ModuleScanResult_t> SignatureScan(std::vector<std::string_view> vecModules, bool bIncludeProcess,
		const std::vector<SignatureData_t> vecSignatures, size_t sizePerModuleLimit = SIZE_MAX);

	/**
	 * Scan the current process and its modules for all instances of the given signatures.
	 * 
	 * \returns A vector of ModuleScanResult_t that give vectors of pointers to instructions and the module it belongs to.
	 * 
	 * \param vecSignatures A vector of SignatureData_t to scan all modules with.
	 * 
	 * \param sizePerModuleLimit The limit of instructions to collect per module.
	 */
	std::vector<ModuleScanResult_t> SignatureScan(const std::vector<SignatureData_t> vecSignatures, size_t sizePerModuleLimit = SIZE_MAX);

	/**
	 * Wrapper for MinHook.
	 */
	template<typename Fn_t>
	class Hook
	{
	private:
		// The target address being hooked.
		void* m_pTarget = nullptr;

		// Storage for the original function pointer.
		Fn_t m_fnOriginal = nullptr;
	public:
		Hook() {};

		/**
		 * Call the original function.
		 */
		template <typename... TArgs>
		auto operator()(TArgs... args)
		{
			static_assert(std::is_invocable_v<Fn_t, TArgs...>, "Args dont match function type!");
			if (!m_fnOriginal)
				throw std::runtime_error("Attempted to call the original function of an uninitialized hook.");

			return m_fnOriginal(std::forward<TArgs>(args)...);
		};

		/**
		 * Disable an already started hook; will not fully remove the hook.
		 * 
		 * \see MH_DisableHook
		 */
		inline MH_STATUS Disable() const
		{
			return MH_DisableHook(m_pTarget);
		};

		/**
		 * Re-enable an already started hook.
		 * 
		 * \see MH_EnableHook
		 */
		inline MH_STATUS Enable() const
		{
			return MH_EnableHook(m_pTarget);
		};

		/**
		 * Disable and remove an installed hook.
		 * 
		 * \see Hook::Disable
		 * \see MH_RemoveHook
		 */
		inline MH_STATUS Remove()
		{
			MH_STATUS eStatus = Disable();

			if (eStatus != MH_OK && eStatus != MH_ERROR_DISABLED)
				return eStatus;

			eStatus = MH_RemoveHook(m_pTarget);
			if (eStatus == MH_OK)
				m_pTarget = nullptr;

			return eStatus;
		};

		/**
		 * Hook an address.
		 * 
		 * \param fnDetour The function that will be called instead of the original function.
		 * 
		 * \param lpAddressThe address of the function to be hooked.
		 * 
		 * \see MH_CreateHook
		 */
		inline MH_STATUS Start(Fn_t fnDetour, void* lpAddress)
		{
			if (MH_STATUS eStatus = MH_CreateHook(lpAddress, fnDetour, reinterpret_cast<void**>(&m_fnOriginal)); eStatus != MH_OK)
				return eStatus;

			m_pTarget = lpAddress;
			return Enable();
		};

		/**
		 * Hook a virtual method table.
		 * 
		 * \param fnDetour The function that will be called instead of the original function.
		 * 
		 * \param lpAddress The virtual method table to hook.
		 * 
		 * \param iIndex The index of the method in the virtual method table to hook.
		 * 
		 * \see MH_CreateHook
		 */
		inline MH_STATUS Start(Fn_t fnDetour, void* lpAddress, size_t iIndex)
		{
			return Start(fnDetour, GetVirtualMethod(lpAddress, iIndex));
		};
	};
}

// This is a dirty hack btw.
template<size_t sizeArray>
inline Memory::SignatureData_t::Span_t ConvertSignatureArrayToVector(std::array<Memory::SignatureData_t::Span_t::value_type, sizeArray> aSignature)
{
	return { aSignature.begin(), aSignature.end() };
}
