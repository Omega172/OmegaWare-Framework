#include "Engines/Unreal/RpcValidate.hpp"

#if OMEGAWARE_ENGINE_UNREAL

#include "Core/Log.hpp"
#include "Engines/Unreal/ValidateThunk.hpp"
#include "Memory/Safety.hpp"

#include "OmegaRenderHook/Core/Guarded.hpp"

#include <Windows.h>

#include <chrono>
#include <cstring>
#include <mutex>

namespace OmegaWare::Unreal
{
	namespace
	{
		constexpr std::uint8_t kReturnTrue[] = { 0xB0, 0x01, 0xC3 };

		[[nodiscard]] bool WriteSlot(void** ppSlot, void* pValue)
		{
			DWORD uProtect = 0;
			if (!VirtualProtect(ppSlot, sizeof(void*), PAGE_READWRITE, &uProtect))
				return false;
			InterlockedExchangePointer(ppSlot, pValue);
			VirtualProtect(ppSlot, sizeof(void*), uProtect, &uProtect);
			return true;
		}
	}

	std::string_view ToString(EValidateStatus eStatus)
	{
		switch (eStatus)
		{
		case EValidateStatus::Ready: return "ready";
		case EValidateStatus::Failed: return "failed";
		case EValidateStatus::Faulted: return "faulted";
		default: return "unresolved";
		}
	}

	bool IsNetHost()
	{
		SDK::UWorld* pWorld = GetWorld();
		if (!Memory::IsValidObjectPtr(pWorld))
			return false;
		SDK::UNetDriver* pDriver = OW_FIELD(pWorld, NetDriver);
		return Memory::IsValidObjectPtr(pDriver) && !OW_FIELD(pDriver, ServerConnection);
	}

	std::optional<std::size_t> FindValidateOffset(SDK::UFunction* pFunction)
	{
		using enum SDK::EFunctionFlags;
		constexpr auto uRequired = static_cast<std::uint32_t>(Net) | static_cast<std::uint32_t>(NetServer) | static_cast<std::uint32_t>(NetValidate) | static_cast<std::uint32_t>(Native);
		if (!Memory::IsValidObjectPtr(pFunction) || (static_cast<std::uint32_t>(pFunction->FunctionFlags) & uRequired) != uRequired)
			return std::nullopt;
		const auto* pThunk = reinterpret_cast<const std::uint8_t*>(pFunction->ExecFunction);
		if (!pThunk || !Memory::IsExecutable(pThunk))
			return std::nullopt;
		const std::string strName = pFunction->GetName() + "_Validate";
		return FindValidateCall(pThunk, kValidateThunkScan, std::wstring(strName.begin(), strName.end()));
	}

	clValidateOverrideBase::clValidateOverrideBase(std::string_view strFunction, void* pDetour) : m_strFunction(strFunction), m_pDetour(pDetour) {}

	bool clValidateOverrideBase::Resolve()
	{
		m_pFunction = SDK::UObject::FindObject<SDK::UFunction>("Function " + m_strFunction, SDK::EClassCastFlags::Function);
		if (!m_pFunction)
			return false;
		const std::optional<std::size_t> uOffset = FindValidateOffset(m_pFunction);
		if (!uOffset)
		{
			LogError("rpc validate: could not find the _Validate call in {}'s exec thunk, override disabled", m_strFunction);
			m_eStatus = EValidateStatus::Failed;
			return false;
		}
		m_uOffset = *uOffset;
		m_eStatus = EValidateStatus::Ready;
		LogInfo("rpc validate: {}_Validate is virtual at +{:#x}", m_strFunction, m_uOffset);
		return true;
	}

	bool clValidateOverrideBase::Attach(const SDK::UObject* pObject)
	{
		const EValidateStatus eStatus = GetStatus();
		if (eStatus == EValidateStatus::Failed || eStatus == EValidateStatus::Faulted)
			return false;
		if (eStatus == EValidateStatus::Unresolved && !Resolve())
			return false;
		if (!Memory::IsValidObjectPtr(pObject) || !pObject->IsA(static_cast<SDK::UClass*>(m_pFunction->Outer)))
			return false;

		void** ppVTable = *reinterpret_cast<void** const*>(pObject);
		{
			std::shared_lock lckPatched(m_mtxPatched);
			if (m_mapPatched.contains(ppVTable) || m_setTrivial.contains(ppVTable))
				return true;
		}
		const std::size_t uIndex = m_uOffset / sizeof(void*);
		if (Memory::GetVirtualMethodCount(pObject) <= uIndex)
		{
			LogError("rpc validate: {} has no vtable slot +{:#x}, override disabled", pObject->GetName(), m_uOffset);
			m_eStatus = EValidateStatus::Failed;
			return false;
		}
		void** ppSlot = ppVTable + uIndex;
		void* pOriginal = *ppSlot;
		if (!Memory::IsExecutable(pOriginal))
		{
			m_eStatus = EValidateStatus::Failed;
			return false;
		}

		std::unique_lock lckPatched(m_mtxPatched);
		if (Memory::IsReadable(pOriginal, sizeof(kReturnTrue)) && std::memcmp(pOriginal, kReturnTrue, sizeof(kReturnTrue)) == 0)
		{
			m_setTrivial.insert(ppVTable);
			LogInfo("rpc validate: {}_Validate always passes for {}, nothing to override", m_strFunction, pObject->Class->GetName());
			return true;
		}
		if (!WriteSlot(ppSlot, m_pDetour))
		{
			LogError("rpc validate: could not patch {}'s vtable for {}", pObject->Class->GetName(), m_strFunction);
			m_eStatus = EValidateStatus::Failed;
			return false;
		}
		m_mapPatched.emplace(ppVTable, pOriginal);
		LogInfo("rpc validate: overriding {}_Validate for {}", m_strFunction, pObject->Class->GetName());
		return true;
	}

	void clValidateOverrideBase::DetachAll()
	{
		std::unordered_map<void**, void*> mapPatched;
		{
			std::shared_lock lckPatched(m_mtxPatched);
			mapPatched = m_mapPatched;
		}
		if (mapPatched.empty())
			return;
		const std::size_t uIndex = m_uOffset / sizeof(void*);
		for (const auto& [ppVTable, pOriginal] : mapPatched)
			if (!WriteSlot(ppVTable + uIndex, pOriginal))
				LogError("rpc validate: could not restore a vtable slot for {}", m_strFunction);
		if (!Hooks::WaitForCalls(std::chrono::milliseconds(500)))
			LogWarning("rpc validate: {} restored while calls were still in flight", m_strFunction);
		std::unique_lock lckPatched(m_mtxPatched);
		m_mapPatched.clear();
		m_setTrivial.clear();
		LogInfo("rpc validate: restored {}_Validate", m_strFunction);
	}

	void* clValidateOverrideBase::OriginalFor(const SDK::UObject* pObject) const
	{
		void** ppVTable = *reinterpret_cast<void** const*>(pObject);
		std::shared_lock lckPatched(m_mtxPatched);
		const auto itPatched = m_mapPatched.find(ppVTable);
		return itPatched != m_mapPatched.end() ? itPatched->second : nullptr;
	}

	bool clValidateOverrideBase::RunPolicy(const std::function<bool()>& fnPolicy)
	{
		if (GetStatus() != EValidateStatus::Ready)
			return false;
		bool bAllow = false;
		OmegaRenderHook::stFault stFault;
		auto fnInvoke = [&] { bAllow = fnPolicy(); };
		if (!OmegaRenderHook::InvokeGuarded(fnInvoke, stFault))
		{
			m_eStatus = EValidateStatus::Faulted;
			LogError("rpc validate: the policy for {} faulted, stock validation only from now on", m_strFunction);
			return false;
		}
		if (bAllow)
			m_uOverrides.fetch_add(1, std::memory_order_relaxed);
		return bAllow;
	}
}

#endif
