#include "Engines/Unreal/Field.hpp"

#if OMEGAWARE_ENGINE_UNREAL

#include "Core/Log.hpp"
#include "Engines/Unreal/Resolver.hpp"
#include "Engines/Unreal/Unreal.hpp"

#include "OmegaRenderHook/Core/Guarded.hpp"

#include <mutex>
#include <optional>

namespace OmegaWare::Unreal
{
	namespace
	{
		std::mutex g_mtxFields;
		std::vector<stFieldInfo> g_aFields;

		[[nodiscard]] std::optional<std::size_t> FindReflectedOffset(SDK::UClass* pClass, std::string_view strMember)
		{
			for (const SDK::UStruct* pStruct = pClass; pStruct; pStruct = pStruct->SuperStruct)
				for (const SDK::FField* pField = pStruct->ChildProperties; pField; pField = pField->Next)
					if (pField->Name.ToString() == strMember)
						return static_cast<std::size_t>(static_cast<const SDK::FProperty*>(pField)->Offset);
			return std::nullopt;
		}
	}

	std::size_t ResolveFieldOffset(SDK::UClass* pClass, std::string_view strMember, std::size_t uSdkOffset)
	{
		stFieldInfo stInfo{ {}, std::string(strMember), uSdkOffset, uSdkOffset, EFieldStatus::NoClass };
		if (!Resolver::IsResolved() || !pClass)
			LogWarning("unreal: the class owning {} was not found, using the SDK offset 0x{:X}", strMember, uSdkOffset);
		else
		{
			stInfo.strClass = pClass->GetName();
			std::optional<std::size_t> optOffset;
			OmegaRenderHook::stFault stFault;
			auto fnFind = [&] { optOffset = FindReflectedOffset(pClass, strMember); };
			if (!OmegaRenderHook::InvokeGuarded(fnFind, stFault))
				LogError("unreal: reading the reflection data of {} faulted (code 0x{:08X})", stInfo.strClass, stFault.uCode);
			if (!optOffset)
			{
				stInfo.eStatus = EFieldStatus::NotReflected;
				LogWarning("unreal: {}::{} is not a reflected property of the game, using the SDK offset 0x{:X}", stInfo.strClass, strMember, uSdkOffset);
			}
			else if (*optOffset != uSdkOffset)
			{
				stInfo.eStatus = EFieldStatus::Moved;
				stInfo.uGameOffset = *optOffset;
				LogWarning("unreal: {}::{} moved from 0x{:X} in the SDK to 0x{:X} in the game, using the game's offset", stInfo.strClass, strMember, uSdkOffset, *optOffset);
			}
			else
				stInfo.eStatus = EFieldStatus::Matches;
		}
		std::scoped_lock lckFields(g_mtxFields);
		g_aFields.push_back(stInfo);
		return stInfo.uGameOffset;
	}

	std::vector<stFieldInfo> ResolvedFields()
	{
		std::scoped_lock lckFields(g_mtxFields);
		return g_aFields;
	}
}

#endif
