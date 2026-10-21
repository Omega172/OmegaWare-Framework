#include "Engines/Unity/Mono.hpp"
#include "Core/Log.hpp"
#include "Memory/Memory.hpp"

#include <Windows.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <mutex>
#include <thread>

namespace OmegaWare::Mono
{
	namespace
	{
		using FnForeachCallback = void (*)(void* pData, void* pUserData);

		struct stExports
		{
			MonoDomain* (*pfnGetRootDomain)() = nullptr;
			MonoThread* (*pfnThreadAttach)(MonoDomain*) = nullptr;
			void (*pfnThreadDetach)(MonoThread*) = nullptr;
			void (*pfnAssemblyForeach)(FnForeachCallback, void*) = nullptr;
			MonoImage* (*pfnAssemblyGetImage)(MonoAssembly*) = nullptr;
			const char* (*pfnImageGetName)(MonoImage*) = nullptr;
			MonoClass* (*pfnClassFromName)(MonoImage*, const char*, const char*) = nullptr;
			MonoMethod* (*pfnClassGetMethodFromName)(MonoClass*, const char*, int) = nullptr;
			void* (*pfnCompileMethod)(MonoMethod*) = nullptr;
			MonoClass* (*pfnMethodGetClass)(MonoMethod*) = nullptr;
			MonoClassField* (*pfnClassGetFieldFromName)(MonoClass*, const char*) = nullptr;
			std::uint32_t (*pfnFieldGetOffset)(MonoClassField*) = nullptr;
			void (*pfnFieldGetValue)(MonoObject*, MonoClassField*, void*) = nullptr;
			void (*pfnFieldSetValue)(MonoObject*, MonoClassField*, void*) = nullptr;
			MonoVTable* (*pfnClassVTable)(MonoDomain*, MonoClass*) = nullptr;
			void* (*pfnVTableGetStaticFieldData)(MonoVTable*) = nullptr;
			void (*pfnRuntimeClassInit)(MonoVTable*) = nullptr;
			MonoObject* (*pfnRuntimeInvoke)(MonoMethod*, void*, void**, MonoObject**) = nullptr;
			MonoString* (*pfnStringNew)(MonoDomain*, const char*) = nullptr;
			char* (*pfnStringToUtf8)(MonoString*) = nullptr;
			void (*pfnFree)(void*) = nullptr;
			void* (*pfnObjectUnbox)(MonoObject*) = nullptr;
		};

		constexpr std::array<std::string_view, 3> kModuleNames = { "mono-2.0-bdwgc.dll", "mono-2.0-sgen.dll", "mono.dll" };

		std::mutex g_mtxInitialize;
		stExports g_stExports;
		std::string g_strModule;
		bool g_bAvailable = false;
		thread_local MonoThread* t_pAttached = nullptr;

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
	}

	bool Initialize()
	{
		std::scoped_lock lckInitialize(g_mtxInitialize);
		if (g_bAvailable)
			return true;

		HMODULE hModule = nullptr;
		for (const Memory::stModule& rModule : Memory::GetModules())
		{
			if (std::ranges::any_of(kModuleNames, [&](std::string_view strName) { return EqualsInsensitive(rModule.strName, strName); }))
			{
				hModule = rModule.hModule;
				g_strModule = rModule.strName;
				break;
			}
		}
		if (!hModule)
		{
			LogWarning("no mono runtime is loaded in this process");
			return false;
		}

		std::string strMissing;
		stExports& r = g_stExports;
		bool bComplete = true;
		bComplete &= Resolve(hModule, "mono_get_root_domain", r.pfnGetRootDomain, strMissing);
		bComplete &= Resolve(hModule, "mono_thread_attach", r.pfnThreadAttach, strMissing);
		bComplete &= Resolve(hModule, "mono_thread_detach", r.pfnThreadDetach, strMissing);
		bComplete &= Resolve(hModule, "mono_assembly_foreach", r.pfnAssemblyForeach, strMissing);
		bComplete &= Resolve(hModule, "mono_assembly_get_image", r.pfnAssemblyGetImage, strMissing);
		bComplete &= Resolve(hModule, "mono_image_get_name", r.pfnImageGetName, strMissing);
		bComplete &= Resolve(hModule, "mono_class_from_name", r.pfnClassFromName, strMissing);
		bComplete &= Resolve(hModule, "mono_class_get_method_from_name", r.pfnClassGetMethodFromName, strMissing);
		bComplete &= Resolve(hModule, "mono_compile_method", r.pfnCompileMethod, strMissing);
		bComplete &= Resolve(hModule, "mono_method_get_class", r.pfnMethodGetClass, strMissing);
		bComplete &= Resolve(hModule, "mono_class_get_field_from_name", r.pfnClassGetFieldFromName, strMissing);
		bComplete &= Resolve(hModule, "mono_field_get_offset", r.pfnFieldGetOffset, strMissing);
		bComplete &= Resolve(hModule, "mono_field_get_value", r.pfnFieldGetValue, strMissing);
		bComplete &= Resolve(hModule, "mono_field_set_value", r.pfnFieldSetValue, strMissing);
		bComplete &= Resolve(hModule, "mono_class_vtable", r.pfnClassVTable, strMissing);
		bComplete &= Resolve(hModule, "mono_vtable_get_static_field_data", r.pfnVTableGetStaticFieldData, strMissing);
		bComplete &= Resolve(hModule, "mono_runtime_class_init", r.pfnRuntimeClassInit, strMissing);
		bComplete &= Resolve(hModule, "mono_runtime_invoke", r.pfnRuntimeInvoke, strMissing);
		bComplete &= Resolve(hModule, "mono_string_new", r.pfnStringNew, strMissing);
		bComplete &= Resolve(hModule, "mono_string_to_utf8", r.pfnStringToUtf8, strMissing);
		bComplete &= Resolve(hModule, "mono_free", r.pfnFree, strMissing);
		bComplete &= Resolve(hModule, "mono_object_unbox", r.pfnObjectUnbox, strMissing);
		if (!bComplete)
		{
			LogError("{} is missing exports: {}", g_strModule, strMissing);
			g_stExports = {};
			return false;
		}
		if (!r.pfnGetRootDomain())
		{
			LogWarning("{} is loaded but has no root domain yet", g_strModule);
			g_stExports = {};
			return false;
		}

		g_bAvailable = true;
		AttachThread();
		LogInfo("mono runtime ready ({})", g_strModule);
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
		if (!t_pAttached)
			t_pAttached = g_stExports.pfnThreadAttach(g_stExports.pfnGetRootDomain());
		return t_pAttached != nullptr;
	}

	MonoDomain* GetRootDomain()
	{
		return g_bAvailable ? g_stExports.pfnGetRootDomain() : nullptr;
	}

	MonoImage* GetImage(std::string_view strAssembly)
	{
		if (!AttachThread())
			return nullptr;
		struct stSearch
		{
			std::string_view strName;
			MonoImage* pImage = nullptr;
		} stState{ strAssembly };
		g_stExports.pfnAssemblyForeach([](void* pData, void* pUserData) {
			auto& rState = *static_cast<stSearch*>(pUserData);
			if (rState.pImage)
				return;
			MonoImage* pImage = g_stExports.pfnAssemblyGetImage(static_cast<MonoAssembly*>(pData));
			const char* szName = pImage ? g_stExports.pfnImageGetName(pImage) : nullptr;
			if (szName && EqualsInsensitive(szName, rState.strName))
				rState.pImage = pImage;
		}, &stState);
		if (!stState.pImage)
			LogWarning("mono assembly \"{}\" is not loaded", strAssembly);
		return stState.pImage;
	}

	MonoClass* GetClass(std::string_view strNamespace, std::string_view strName, std::string_view strAssembly)
	{
		MonoImage* pImage = GetImage(strAssembly);
		if (!pImage)
			return nullptr;
		MonoClass* pClass = g_stExports.pfnClassFromName(pImage, std::string(strNamespace).c_str(), std::string(strName).c_str());
		if (!pClass)
			LogWarning("mono class \"{}{}{}\" not found in {}", strNamespace, strNamespace.empty() ? "" : ".", strName, strAssembly);
		return pClass;
	}

	MonoMethod* GetMethod(MonoClass* pClass, std::string_view strName, int iParameterCount)
	{
		if (!pClass || !AttachThread())
			return nullptr;
		MonoMethod* pMethod = g_stExports.pfnClassGetMethodFromName(pClass, std::string(strName).c_str(), iParameterCount);
		if (!pMethod)
			LogWarning("mono method \"{}\" ({} parameters) not found", strName, iParameterCount);
		return pMethod;
	}

	void* CompileMethod(MonoMethod* pMethod)
	{
		if (!pMethod || !AttachThread())
			return nullptr;
		return g_stExports.pfnCompileMethod(pMethod);
	}

	void* GetCompiledMethod(std::string_view strNamespace, std::string_view strClass, std::string_view strMethod, int iParameterCount, std::string_view strAssembly)
	{
		return CompileMethod(GetMethod(GetClass(strNamespace, strClass, strAssembly), strMethod, iParameterCount));
	}

	MonoClass* GetMethodClass(MonoMethod* pMethod)
	{
		return pMethod && AttachThread() ? g_stExports.pfnMethodGetClass(pMethod) : nullptr;
	}

	MonoClassField* GetField(MonoClass* pClass, std::string_view strName)
	{
		if (!pClass || !AttachThread())
			return nullptr;
		MonoClassField* pField = g_stExports.pfnClassGetFieldFromName(pClass, std::string(strName).c_str());
		if (!pField)
			LogWarning("mono field \"{}\" not found", strName);
		return pField;
	}

	std::optional<std::uint32_t> GetFieldOffset(MonoClassField* pField)
	{
		if (!pField || !AttachThread())
			return std::nullopt;
		return g_stExports.pfnFieldGetOffset(pField);
	}

	void* GetStaticFieldData(MonoClass* pClass)
	{
		if (!pClass || !AttachThread())
			return nullptr;
		MonoVTable* pVTable = g_stExports.pfnClassVTable(g_stExports.pfnGetRootDomain(), pClass);
		if (!pVTable)
			return nullptr;
		g_stExports.pfnRuntimeClassInit(pVTable);
		return g_stExports.pfnVTableGetStaticFieldData(pVTable);
	}

	void GetFieldValue(MonoObject* pObject, MonoClassField* pField, void* pOut)
	{
		if (pObject && pField && pOut && AttachThread())
			g_stExports.pfnFieldGetValue(pObject, pField, pOut);
	}

	void SetFieldValue(MonoObject* pObject, MonoClassField* pField, void* pValue)
	{
		if (pObject && pField && pValue && AttachThread())
			g_stExports.pfnFieldSetValue(pObject, pField, pValue);
	}

	MonoObject* Invoke(MonoMethod* pMethod, void* pInstance, void** ppArguments)
	{
		if (!pMethod || !AttachThread())
			return nullptr;
		MonoObject* pException = nullptr;
		MonoObject* pResult = g_stExports.pfnRuntimeInvoke(pMethod, pInstance, ppArguments, &pException);
		if (pException)
		{
			LogWarning("managed exception while invoking a mono method");
			return nullptr;
		}
		return pResult;
	}

	MonoString* NewString(std::string_view strText)
	{
		if (!AttachThread())
			return nullptr;
		return g_stExports.pfnStringNew(g_stExports.pfnGetRootDomain(), std::string(strText).c_str());
	}

	std::string ToString(MonoString* pString)
	{
		if (!pString || !AttachThread())
			return {};
		char* szText = g_stExports.pfnStringToUtf8(pString);
		if (!szText)
			return {};
		std::string strResult(szText);
		g_stExports.pfnFree(szText);
		return strResult;
	}

	void* Unbox(MonoObject* pObject)
	{
		return pObject && AttachThread() ? g_stExports.pfnObjectUnbox(pObject) : nullptr;
	}

	bool LogDiagnostics()
	{
		if (!AttachThread())
			return false;
		bool bPassed = true;

		MonoClass* pEnvironment = GetClass("System", "Environment", "mscorlib");
		MonoMethod* pProcessorCount = GetMethod(pEnvironment, "get_ProcessorCount", 0);
		MonoObject* pBoxed = Invoke(pProcessorCount, nullptr, nullptr);
		const auto* pCount = static_cast<const int*>(Unbox(pBoxed));
		const int iExpected = static_cast<int>(std::thread::hardware_concurrency());
		const bool bInvoke = pCount && *pCount == iExpected;
		bPassed &= bInvoke;
		LogInfo("mono check: Environment.ProcessorCount = {} (expected {}) {}", pCount ? *pCount : -1, iExpected, bInvoke ? "ok" : "FAILED");

		void* pCompiled = CompileMethod(pProcessorCount);
		const bool bCompile = pCompiled && Memory::IsExecutable(pCompiled);
		bPassed &= bCompile;
		LogInfo("mono check: compiled get_ProcessorCount at {} {}", pCompiled, bCompile ? "ok" : "FAILED");

		const std::optional<bool> optLittleEndian = GetStaticField<bool>(GetClass("System", "BitConverter", "mscorlib"), "IsLittleEndian");
		const bool bStatic = optLittleEndian.value_or(false);
		bPassed &= bStatic;
		LogInfo("mono check: BitConverter.IsLittleEndian = {} {}", optLittleEndian ? (*optLittleEndian ? "true" : "false") : "unreadable", bStatic ? "ok" : "FAILED");

		const bool bGameAssembly = GetImage(kDefaultAssembly) != nullptr;
		LogInfo("mono check: {} {}", kDefaultAssembly, bGameAssembly ? "loaded" : "not loaded");
		LogInfo("mono checks {}", bPassed ? "passed" : "FAILED");
		return bPassed;
	}
}
