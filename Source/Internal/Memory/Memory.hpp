#pragma once
#include "pch.h"

#include "Windows/WindowsMemory.hpp"
#include "../../Shared/Memory/PointerSafety.hpp"

#define Signature(n) (ConvertSignatureArrayToVector(Memory::SignatureData_t::Conversion_t::build<stb::fixed_string{n}>::value))

namespace Memory
{
	/**
	 * \warning Is more expensive than std::strlen!
	 */
	size_t Strlen(const char* lpAddress, size_t dwMaxSize = 1024);

	/**
	 * \warning Is more expensive than std::wcslen!
	 */
	size_t Wcslen(const wchar_t* lpAddress, size_t dwMaxSize = 1024);

	struct InterfaceRegistry_t {
		void* (cdecl* m_fnCreate)();
		const char* m_szName;
		InterfaceRegistry_t* m_pNext;
	};

	void EnumerateInterfaces(std::string_view svModuleName, std::function<bool(InterfaceRegistry_t*)> fn);

	void* CreateInterface(std::string_view svModuleName, std::string_view svInterfaceName);

	struct SignatureData_t {
		using Span_t = std::vector<int16_t>;
		using Conversion_t = stb::basic_hex_string_array_conversion<' ', '?', Span_t::value_type, -1>;

		Span_t aSignature;
		std::function<uintptr_t(uintptr_t)> CorrectReturnAddressFunc;
	};

	void* SignatureScan(const std::string_view svModuleName, const SignatureData_t::Span_t aSignature);

	void* SignatureScan(const HMODULE hModule, const SignatureData_t::Span_t aSignature);

	void* SignatureScan(const std::string_view svModuleName, const std::vector<SignatureData_t> vecSignatures);

	void* SignatureScan(const HMODULE hModule, const std::vector<SignatureData_t> vecSignatures);

	struct ModuleScanResult_t {
		std::string m_sModuleName;
		std::vector<void*> m_vecPointers;
	};

	std::vector<ModuleScanResult_t> SignatureScan(std::vector<std::string_view> vecModules, bool bIncludeProcess,
		const std::vector<SignatureData_t> vecSignatures, size_t sizePerModuleLimit = SIZE_MAX);

	std::vector<ModuleScanResult_t> SignatureScan(const std::vector<SignatureData_t> vecSignatures, size_t sizePerModuleLimit = SIZE_MAX);
}

template<size_t sizeArray>
inline Memory::SignatureData_t::Span_t ConvertSignatureArrayToVector(std::array<Memory::SignatureData_t::Span_t::value_type, sizeArray> aSignature)
{
	return { aSignature.begin(), aSignature.end() };
}
