#pragma once

#include "pch.h"

#define STB_OMIT_TESTS
#include "../Libs/StringToBytes/stb.h"

#include "Windows/WindowsMemory.h"

#define IsValidObjectPtr(lpAddress) (Memory::IsReadable(lpAddress, sizeof(*lpAddress)))
#define IsValidPtr(lpAddress) (Memory::IsReadable(lpAddress, sizeof(void*)))

#define PtrOffset(ptr, n)         (ptr + n)
#define PtrDereference(ptr)       (*reinterpret_cast<void**>(ptr))
#define PtrJmp(ptr)               (PtrOffset(ptr, sizeof(int) + *reinterpret_cast<int*>(ptr)))
#define PtrOffsetJmp(ptr, n1, n2) (PtrOffset(PtrJmp(PtrOffset(ptr, n1)), n2))

#define Signature(n) Memory::SignatureConversion_t::<stb::fixed_string{n}>::value

namespace Memory
{
	// Checks to see if memory is readable to prevent access violations, can be expensive
	bool IsReadable(const void* lpAddress, size_t dwLength);

	// Gets the length of a string while doing readable checks, more expensive than std::strlen
	size_t Strlen(const char* lpAddress, size_t dwMaxSize = 1024);
	// Gets the length of a wstring while doing readable checks, more expensive than std::wcslen
	size_t Wcslen(const wchar_t* lpAddress, size_t dwMaxSize = 1024);

	typedef struct InterfaceRegistry_t {
		using CreateFunc = void* (*)();
		CreateFunc Create;
		const char* szName;
		InterfaceRegistry_t* pNext;
	} InterfaceRegistry_t;

	using EnumerateInterfacesFunc = std::function<bool(InterfaceRegistry_t*)>;

	void EnumerateInterfaces(std::string_view sModuleName, EnumerateInterfacesFunc fn);

	void* CreateInterface(std::string_view sModuleName, std::string_view sInterfaceName);


	// Get the size of the virtual method table
	size_t GetVirtualMethodTableSize(void* lpAddress);

	// Gets method from virtual method table
	void* GetVirtualMethod(void* lpAddress, size_t index);

	// Types used for Signature conversion and arrays, use STB (string to bytes) to convert strings to spans that contain SignatureElement_t
	using SignatureElement_t = short;
	constexpr SignatureElement_t SignatureWildcardConvertedValue = -1;
	using SignatureConversion_t = stb::basic_hex_string_array_conversion<' ', '?', SignatureElement_t, SignatureWildcardConvertedValue>;
	using SignatureSpan_t = std::span<const SignatureElement_t>;

	// Scans module for memory signature, returning the found address
	void* SignatureScan(LPMODULEINFO pModuleInfo, SignatureSpan_t aSignature);

	// Type used for MultiSignatureScan function, contains signature and function to correct the return address
	typedef struct SignatureData_t {
		SignatureSpan_t aSignature;
		// Used for multiscan so different signatures that would need to be corrected differently can be scanned, can be a nullptr
		std::function<void* (void*)> CorrectReturnAddressFunc;
	} SignatureData_t;

	// Scans module for signatures and will call the sig's CorrectReturnAddressFunc before returning the found address
	void* MultiSignatureScan(std::string_view sModuleName, std::vector<SignatureData_t*> vecSignatures);

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