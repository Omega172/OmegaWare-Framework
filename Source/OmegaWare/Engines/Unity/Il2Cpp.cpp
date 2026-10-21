#include "Engines/Unity/Il2Cpp.hpp"
#include "Core/Log.hpp"
#include "Memory/Memory.hpp"

#include <Windows.h>

#include <algorithm>
#include <cctype>
#include <mutex>
#include <thread>

namespace OmegaWare::Il2Cpp
{
	namespace
	{
		struct stExports
		{
			Il2CppDomain* (*pfnDomainGet)() = nullptr;
			Il2CppThread* (*pfnThreadAttach)(Il2CppDomain*) = nullptr;
			void (*pfnThreadDetach)(Il2CppThread*) = nullptr;
			Il2CppThread* (*pfnThreadCurrent)() = nullptr;
			const Il2CppAssembly** (*pfnDomainGetAssemblies)(const Il2CppDomain*, std::size_t*) = nullptr;
			const Il2CppImage* (*pfnAssemblyGetImage)(const Il2CppAssembly*) = nullptr;
			const char* (*pfnImageGetName)(const Il2CppImage*) = nullptr;
			Il2CppClass* (*pfnClassFromName)(const Il2CppImage*, const char*, const char*) = nullptr;
			const MethodInfo* (*pfnClassGetMethodFromName)(Il2CppClass*, const char*, int) = nullptr;
			Il2CppClass* (*pfnMethodGetClass)(const MethodInfo*) = nullptr;
			FieldInfo* (*pfnClassGetFieldFromName)(Il2CppClass*, const char*) = nullptr;
			std::size_t (*pfnFieldGetOffset)(FieldInfo*) = nullptr;
			void (*pfnFieldGetValue)(Il2CppObject*, FieldInfo*, void*) = nullptr;
			void (*pfnFieldSetValue)(Il2CppObject*, FieldInfo*, void*) = nullptr;
			void (*pfnFieldStaticGetValue)(FieldInfo*, void*) = nullptr;
			void (*pfnFieldStaticSetValue)(FieldInfo*, void*) = nullptr;
			void (*pfnRuntimeClassInit)(Il2CppClass*) = nullptr;
			Il2CppObject* (*pfnRuntimeInvoke)(const MethodInfo*, void*, void**, Il2CppObject**) = nullptr;
			Il2CppString* (*pfnStringNew)(const char*) = nullptr;
			std::int32_t (*pfnStringLength)(Il2CppString*) = nullptr;
			char16_t* (*pfnStringChars)(Il2CppString*) = nullptr;
			void* (*pfnObjectUnbox)(Il2CppObject*) = nullptr;
		};

		constexpr std::string_view kModuleName = "GameAssembly.dll";

		std::mutex g_mtxInitialize;
		stExports g_stExports;
		std::string g_strModule;
		bool g_bAvailable = false;
		thread_local Il2CppThread* t_pAttached = nullptr;

		template <typename T>
		bool Resolve(HMODULE hModule, const char* szName, T& rFunction, std::string& rMissing)
		{
			rFunction = reinterpret_cast<T>(GetProcAddress(hModule, szName));
			if (rFunction)
				return true;
			rMissing += rMissing.empty() ? szName : std::string(", ") + szName;
			return false;
		}

		[[nodiscard]] bool EqualsInsensitive(std::string_view strLeft, std::string_view strRight)
		{
			return std::ranges::equal(strLeft, strRight, [](char chLeft, char chRight) { return std::tolower(static_cast<unsigned char>(chLeft)) == std::tolower(static_cast<unsigned char>(chRight)); });
		}

		[[nodiscard]] std::string_view WithoutExtension(std::string_view strName)
		{
			if (strName.size() > 4 && EqualsInsensitive(strName.substr(strName.size() - 4), ".dll"))
				strName.remove_suffix(4);
			return strName;
		}
	}

	bool Initialize()
	{
		std::scoped_lock lckInitialize(g_mtxInitialize);
		if (g_bAvailable)
			return true;

		const std::optional<Memory::stModule> optModule = Memory::GetModule(kModuleName);
		if (!optModule)
		{
			LogWarning("no il2cpp runtime ({}) is loaded in this process", kModuleName);
			return false;
		}
		const HMODULE hModule = optModule->hModule;
		g_strModule = optModule->strName;

		std::string strMissing;
		stExports& r = g_stExports;
		bool bComplete = true;
		bComplete &= Resolve(hModule, "il2cpp_domain_get", r.pfnDomainGet, strMissing);
		bComplete &= Resolve(hModule, "il2cpp_thread_attach", r.pfnThreadAttach, strMissing);
		bComplete &= Resolve(hModule, "il2cpp_thread_detach", r.pfnThreadDetach, strMissing);
		bComplete &= Resolve(hModule, "il2cpp_thread_current", r.pfnThreadCurrent, strMissing);
		bComplete &= Resolve(hModule, "il2cpp_domain_get_assemblies", r.pfnDomainGetAssemblies, strMissing);
		bComplete &= Resolve(hModule, "il2cpp_assembly_get_image", r.pfnAssemblyGetImage, strMissing);
		bComplete &= Resolve(hModule, "il2cpp_image_get_name", r.pfnImageGetName, strMissing);
		bComplete &= Resolve(hModule, "il2cpp_class_from_name", r.pfnClassFromName, strMissing);
		bComplete &= Resolve(hModule, "il2cpp_class_get_method_from_name", r.pfnClassGetMethodFromName, strMissing);
		bComplete &= Resolve(hModule, "il2cpp_method_get_class", r.pfnMethodGetClass, strMissing);
		bComplete &= Resolve(hModule, "il2cpp_class_get_field_from_name", r.pfnClassGetFieldFromName, strMissing);
		bComplete &= Resolve(hModule, "il2cpp_field_get_offset", r.pfnFieldGetOffset, strMissing);
		bComplete &= Resolve(hModule, "il2cpp_field_get_value", r.pfnFieldGetValue, strMissing);
		bComplete &= Resolve(hModule, "il2cpp_field_set_value", r.pfnFieldSetValue, strMissing);
		bComplete &= Resolve(hModule, "il2cpp_field_static_get_value", r.pfnFieldStaticGetValue, strMissing);
		bComplete &= Resolve(hModule, "il2cpp_field_static_set_value", r.pfnFieldStaticSetValue, strMissing);
		bComplete &= Resolve(hModule, "il2cpp_runtime_class_init", r.pfnRuntimeClassInit, strMissing);
		bComplete &= Resolve(hModule, "il2cpp_runtime_invoke", r.pfnRuntimeInvoke, strMissing);
		bComplete &= Resolve(hModule, "il2cpp_string_new", r.pfnStringNew, strMissing);
		bComplete &= Resolve(hModule, "il2cpp_string_length", r.pfnStringLength, strMissing);
		bComplete &= Resolve(hModule, "il2cpp_string_chars", r.pfnStringChars, strMissing);
		bComplete &= Resolve(hModule, "il2cpp_object_unbox", r.pfnObjectUnbox, strMissing);
		if (!bComplete)
		{
			LogError("{} is missing exports (they may be obfuscated): {}", g_strModule, strMissing);
			g_stExports = {};
			return false;
		}
		if (!r.pfnDomainGet())
		{
			LogWarning("{} is loaded but has no domain yet", g_strModule);
			g_stExports = {};
			return false;
		}

		g_bAvailable = true;
		AttachThread();
		LogInfo("il2cpp runtime ready ({})", g_strModule);
		return true;
	}

	void Shutdown()
	{
		std::scoped_lock lckInitialize(g_mtxInitialize);
		if (g_bAvailable && t_pAttached)
			g_stExports.pfnThreadDetach(t_pAttached);
		t_pAttached = nullptr;
		g_bAvailable = false;
	}

	bool IsAvailable()
	{
		return g_bAvailable;
	}

	std::string_view GetModuleName()
	{
		return g_strModule;
	}

	bool AttachThread()
	{
		if (!g_bAvailable)
			return false;
		if (!t_pAttached && !g_stExports.pfnThreadCurrent())
			t_pAttached = g_stExports.pfnThreadAttach(g_stExports.pfnDomainGet());
		return t_pAttached != nullptr || g_stExports.pfnThreadCurrent() != nullptr;
	}

	Il2CppDomain* GetDomain()
	{
		return g_bAvailable ? g_stExports.pfnDomainGet() : nullptr;
	}

	const Il2CppImage* GetImage(std::string_view strAssembly)
	{
		if (!AttachThread())
			return nullptr;
		std::size_t uCount = 0;
		const Il2CppAssembly** ppAssemblies = g_stExports.pfnDomainGetAssemblies(g_stExports.pfnDomainGet(), &uCount);
		for (std::size_t i = 0; ppAssemblies && i < uCount; ++i)
		{
			const Il2CppImage* pImage = g_stExports.pfnAssemblyGetImage(ppAssemblies[i]);
			const char* szName = pImage ? g_stExports.pfnImageGetName(pImage) : nullptr;
			if (szName && EqualsInsensitive(WithoutExtension(szName), WithoutExtension(strAssembly)))
				return pImage;
		}
		LogWarning("il2cpp assembly \"{}\" is not loaded", strAssembly);
		return nullptr;
	}

	Il2CppClass* GetClass(std::string_view strNamespace, std::string_view strName, std::string_view strAssembly)
	{
		const Il2CppImage* pImage = GetImage(strAssembly);
		if (!pImage)
			return nullptr;
		Il2CppClass* pClass = g_stExports.pfnClassFromName(pImage, std::string(strNamespace).c_str(), std::string(strName).c_str());
		if (!pClass)
			LogWarning("il2cpp class \"{}{}{}\" not found in {}", strNamespace, strNamespace.empty() ? "" : ".", strName, strAssembly);
		return pClass;
	}

	const MethodInfo* GetMethod(Il2CppClass* pClass, std::string_view strName, int iParameterCount)
	{
		if (!pClass || !AttachThread())
			return nullptr;
		const MethodInfo* pMethod = g_stExports.pfnClassGetMethodFromName(pClass, std::string(strName).c_str(), iParameterCount);
		if (!pMethod)
			LogWarning("il2cpp method \"{}\" ({} parameters) not found", strName, iParameterCount);
		return pMethod;
	}

	void* GetMethodPointer(const MethodInfo* pMethod)
	{
		if (!pMethod)
			return nullptr;
		const std::optional<void*> optPointer = Memory::SafeRead<void*>(reinterpret_cast<std::uintptr_t>(pMethod));
		return optPointer && Memory::IsExecutable(*optPointer) ? *optPointer : nullptr;
	}

	void* GetCompiledMethod(std::string_view strNamespace, std::string_view strClass, std::string_view strMethod, int iParameterCount, std::string_view strAssembly)
	{
		return GetMethodPointer(GetMethod(GetClass(strNamespace, strClass, strAssembly), strMethod, iParameterCount));
	}

	Il2CppClass* GetMethodClass(const MethodInfo* pMethod)
	{
		return pMethod && AttachThread() ? g_stExports.pfnMethodGetClass(pMethod) : nullptr;
	}

	FieldInfo* GetField(Il2CppClass* pClass, std::string_view strName)
	{
		if (!pClass || !AttachThread())
			return nullptr;
		FieldInfo* pField = g_stExports.pfnClassGetFieldFromName(pClass, std::string(strName).c_str());
		if (!pField)
			LogWarning("il2cpp field \"{}\" not found", strName);
		return pField;
	}

	std::optional<std::size_t> GetFieldOffset(FieldInfo* pField)
	{
		if (!pField || !AttachThread())
			return std::nullopt;
		return g_stExports.pfnFieldGetOffset(pField);
	}

	bool GetStaticFieldValue(Il2CppClass* pClass, FieldInfo* pField, void* pOut)
	{
		if (!pClass || !pField || !pOut || !AttachThread())
			return false;
		g_stExports.pfnRuntimeClassInit(pClass);
		g_stExports.pfnFieldStaticGetValue(pField, pOut);
		return true;
	}

	bool SetStaticFieldValue(Il2CppClass* pClass, FieldInfo* pField, void* pValue)
	{
		if (!pClass || !pField || !pValue || !AttachThread())
			return false;
		g_stExports.pfnRuntimeClassInit(pClass);
		g_stExports.pfnFieldStaticSetValue(pField, pValue);
		return true;
	}

	void GetFieldValue(Il2CppObject* pObject, FieldInfo* pField, void* pOut)
	{
		if (pObject && pField && pOut && AttachThread())
			g_stExports.pfnFieldGetValue(pObject, pField, pOut);
	}

	void SetFieldValue(Il2CppObject* pObject, FieldInfo* pField, void* pValue)
	{
		if (pObject && pField && pValue && AttachThread())
			g_stExports.pfnFieldSetValue(pObject, pField, pValue);
	}

	Il2CppObject* Invoke(const MethodInfo* pMethod, void* pInstance, void** ppArguments)
	{
		if (!pMethod || !AttachThread())
			return nullptr;
		Il2CppObject* pException = nullptr;
		Il2CppObject* pResult = g_stExports.pfnRuntimeInvoke(pMethod, pInstance, ppArguments, &pException);
		if (pException)
		{
			LogWarning("managed exception while invoking an il2cpp method");
			return nullptr;
		}
		return pResult;
	}

	Il2CppString* NewString(std::string_view strText)
	{
		if (!AttachThread())
			return nullptr;
		return g_stExports.pfnStringNew(std::string(strText).c_str());
	}

	std::string ToString(Il2CppString* pString)
	{
		if (!pString || !AttachThread())
			return {};
		const char16_t* pChars = g_stExports.pfnStringChars(pString);
		const std::int32_t iLength = g_stExports.pfnStringLength(pString);
		if (!pChars || iLength <= 0)
			return {};
		const int iBytes = WideCharToMultiByte(CP_UTF8, 0, reinterpret_cast<const wchar_t*>(pChars), iLength, nullptr, 0, nullptr, nullptr);
		std::string strResult(static_cast<std::size_t>(std::max(iBytes, 0)), '\0');
		WideCharToMultiByte(CP_UTF8, 0, reinterpret_cast<const wchar_t*>(pChars), iLength, strResult.data(), iBytes, nullptr, nullptr);
		return strResult;
	}

	void* Unbox(Il2CppObject* pObject)
	{
		return pObject && AttachThread() ? g_stExports.pfnObjectUnbox(pObject) : nullptr;
	}

	bool LogDiagnostics()
	{
		if (!AttachThread())
			return false;
		bool bPassed = true;

		const MethodInfo* pProcessorCount = GetMethod(GetClass("System", "Environment", "mscorlib"), "get_ProcessorCount", 0);
		if (pProcessorCount)
		{
			const auto* pCount = static_cast<const int*>(Unbox(Invoke(pProcessorCount, nullptr, nullptr)));
			const int iExpected = static_cast<int>(std::thread::hardware_concurrency());
			const bool bInvoke = pCount && *pCount == iExpected;
			bPassed &= bInvoke;
			LogInfo("il2cpp check: Environment.ProcessorCount = {} (expected {}) {}", pCount ? *pCount : -1, iExpected, bInvoke ? "ok" : "FAILED");

			void* pCompiled = GetMethodPointer(pProcessorCount);
			bPassed &= pCompiled != nullptr;
			LogInfo("il2cpp check: get_ProcessorCount code at {} {}", pCompiled, pCompiled ? "ok" : "FAILED");
		}
		else
			LogInfo("il2cpp check: Environment.get_ProcessorCount was stripped from this build, skipped");

		Il2CppClass* pBitConverter = GetClass("System", "BitConverter", "mscorlib");
		if (pBitConverter && GetField(pBitConverter, "IsLittleEndian"))
		{
			const std::optional<bool> optLittleEndian = GetStaticField<bool>(pBitConverter, "IsLittleEndian");
			const bool bStatic = optLittleEndian.value_or(false);
			bPassed &= bStatic;
			LogInfo("il2cpp check: BitConverter.IsLittleEndian = {} {}", optLittleEndian ? (*optLittleEndian ? "true" : "false") : "unreadable", bStatic ? "ok" : "FAILED");
		}
		else
			LogInfo("il2cpp check: BitConverter.IsLittleEndian was stripped from this build, skipped");

		Il2CppString* pString = NewString("omegaware");
		const bool bString = ToString(pString) == "omegaware";
		bPassed &= bString;
		LogInfo("il2cpp check: string round trip {}", bString ? "ok" : "FAILED");

		const bool bGameAssembly = GetImage(kDefaultAssembly) != nullptr;
		LogInfo("il2cpp check: {} {}", kDefaultAssembly, bGameAssembly ? "loaded" : "not loaded");
		LogInfo("il2cpp checks {}", bPassed ? "passed" : "FAILED");
		return bPassed;
	}
}
