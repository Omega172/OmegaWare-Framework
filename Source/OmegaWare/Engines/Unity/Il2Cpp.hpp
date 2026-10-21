#pragma once

#if OMEGAWARE_ENGINE_UNITY

#include "Memory/Safety.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

struct Il2CppDomain;
struct Il2CppThread;
struct Il2CppAssembly;
struct Il2CppImage;
struct Il2CppClass;
struct MethodInfo;
struct FieldInfo;
struct Il2CppObject;
struct Il2CppString;

namespace OmegaWare::Il2Cpp
{
	inline constexpr std::string_view kDefaultAssembly = "Assembly-CSharp";

	bool Initialize();
	void Shutdown();
	[[nodiscard]] bool IsAvailable();
	[[nodiscard]] std::string_view GetModuleName();
	bool AttachThread();

	[[nodiscard]] Il2CppDomain* GetDomain();
	[[nodiscard]] const Il2CppImage* GetImage(std::string_view strAssembly = kDefaultAssembly);
	[[nodiscard]] Il2CppClass* GetClass(std::string_view strNamespace, std::string_view strName, std::string_view strAssembly = kDefaultAssembly);
	[[nodiscard]] const MethodInfo* GetMethod(Il2CppClass* pClass, std::string_view strName, int iParameterCount = -1);
	[[nodiscard]] void* GetMethodPointer(const MethodInfo* pMethod);
	[[nodiscard]] void* GetCompiledMethod(std::string_view strNamespace, std::string_view strClass, std::string_view strMethod, int iParameterCount = -1, std::string_view strAssembly = kDefaultAssembly);
	[[nodiscard]] Il2CppClass* GetMethodClass(const MethodInfo* pMethod);

	[[nodiscard]] FieldInfo* GetField(Il2CppClass* pClass, std::string_view strName);
	[[nodiscard]] std::optional<std::size_t> GetFieldOffset(FieldInfo* pField);
	bool GetStaticFieldValue(Il2CppClass* pClass, FieldInfo* pField, void* pOut);
	bool SetStaticFieldValue(Il2CppClass* pClass, FieldInfo* pField, void* pValue);
	void GetFieldValue(Il2CppObject* pObject, FieldInfo* pField, void* pOut);
	void SetFieldValue(Il2CppObject* pObject, FieldInfo* pField, void* pValue);

	Il2CppObject* Invoke(const MethodInfo* pMethod, void* pInstance, void** ppArguments);
	[[nodiscard]] Il2CppString* NewString(std::string_view strText);
	[[nodiscard]] std::string ToString(Il2CppString* pString);
	[[nodiscard]] void* Unbox(Il2CppObject* pObject);
	bool LogDiagnostics();

	template <typename T>
	[[nodiscard]] std::optional<T> GetStaticField(Il2CppClass* pClass, std::string_view strField)
	{
		T value{};
		if (!GetStaticFieldValue(pClass, GetField(pClass, strField), &value))
			return std::nullopt;
		return value;
	}

	template <typename T>
	[[nodiscard]] std::optional<T> GetInstanceField(Il2CppObject* pObject, FieldInfo* pField)
	{
		const std::optional<std::size_t> optOffset = GetFieldOffset(pField);
		if (!pObject || !optOffset)
			return std::nullopt;
		return Memory::SafeRead<T>(reinterpret_cast<std::uintptr_t>(pObject) + *optOffset);
	}
}

#endif
