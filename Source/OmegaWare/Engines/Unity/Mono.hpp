#pragma once

#if OMEGAWARE_ENGINE_UNITY

#include "Memory/Safety.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

struct MonoDomain;
struct MonoThread;
struct MonoAssembly;
struct MonoImage;
struct MonoClass;
struct MonoMethod;
struct MonoClassField;
struct MonoVTable;
struct MonoObject;
struct MonoString;

namespace OmegaWare::Mono
{
	inline constexpr std::string_view kDefaultAssembly = "Assembly-CSharp";

	bool Initialize();
	void Shutdown();
	[[nodiscard]] bool IsAvailable();
	[[nodiscard]] std::string_view GetModuleName();
	bool AttachThread();

	[[nodiscard]] MonoDomain* GetRootDomain();
	[[nodiscard]] MonoImage* GetImage(std::string_view strAssembly = kDefaultAssembly);
	[[nodiscard]] MonoClass* GetClass(std::string_view strNamespace, std::string_view strName, std::string_view strAssembly = kDefaultAssembly);
	[[nodiscard]] MonoMethod* GetMethod(MonoClass* pClass, std::string_view strName, int iParameterCount = -1);
	[[nodiscard]] void* CompileMethod(MonoMethod* pMethod);
	[[nodiscard]] void* GetCompiledMethod(std::string_view strNamespace, std::string_view strClass, std::string_view strMethod, int iParameterCount = -1, std::string_view strAssembly = kDefaultAssembly);
	[[nodiscard]] MonoClass* GetMethodClass(MonoMethod* pMethod);

	[[nodiscard]] MonoClassField* GetField(MonoClass* pClass, std::string_view strName);
	[[nodiscard]] std::optional<std::uint32_t> GetFieldOffset(MonoClassField* pField);
	[[nodiscard]] void* GetStaticFieldData(MonoClass* pClass);
	void GetFieldValue(MonoObject* pObject, MonoClassField* pField, void* pOut);
	void SetFieldValue(MonoObject* pObject, MonoClassField* pField, void* pValue);

	MonoObject* Invoke(MonoMethod* pMethod, void* pInstance, void** ppArguments);
	[[nodiscard]] MonoString* NewString(std::string_view strText);
	[[nodiscard]] std::string ToString(MonoString* pString);
	[[nodiscard]] void* Unbox(MonoObject* pObject);
	bool LogDiagnostics();

	template <typename T>
	[[nodiscard]] std::optional<T> GetStaticField(MonoClass* pClass, std::string_view strField)
	{
		void* pData = GetStaticFieldData(pClass);
		const std::optional<std::uint32_t> optOffset = GetFieldOffset(GetField(pClass, strField));
		if (!pData || !optOffset)
			return std::nullopt;
		return Memory::SafeRead<T>(reinterpret_cast<std::uintptr_t>(pData) + *optOffset);
	}

	template <typename T>
	[[nodiscard]] std::optional<T> GetInstanceField(MonoObject* pObject, MonoClassField* pField)
	{
		const std::optional<std::uint32_t> optOffset = GetFieldOffset(pField);
		if (!pObject || !optOffset)
			return std::nullopt;
		return Memory::SafeRead<T>(reinterpret_cast<std::uintptr_t>(pObject) + *optOffset);
	}
}

#endif
