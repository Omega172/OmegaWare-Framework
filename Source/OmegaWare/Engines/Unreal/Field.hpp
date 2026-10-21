#pragma once

#if OMEGAWARE_ENGINE_UNREAL

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace SDK
{
	class UClass;
}

namespace OmegaWare::Unreal
{
	enum class EFieldStatus
	{
		Matches,
		Moved,
		NotReflected,
		NoClass,
	};

	struct stFieldInfo
	{
		std::string strClass;
		std::string strMember;
		std::size_t uSdkOffset = 0;
		std::size_t uGameOffset = 0;
		EFieldStatus eStatus = EFieldStatus::Matches;
	};

	[[nodiscard]] std::size_t ResolveFieldOffset(SDK::UClass* pClass, std::string_view strMember, std::size_t uSdkOffset);
	[[nodiscard]] std::vector<stFieldInfo> ResolvedFields();

	template <typename T, typename O>
	[[nodiscard]] T& FieldAt(O* pObject, std::size_t uOffset)
	{
		return *reinterpret_cast<T*>(reinterpret_cast<std::uint8_t*>(const_cast<std::remove_cv_t<O>*>(pObject)) + uOffset);
	}
}

#define OW_FIELD(pObject, Member)                                                                                                                              \
	(::OmegaWare::Unreal::FieldAt<decltype(std::remove_cvref_t<decltype(*(pObject))>::Member)>((pObject), [] {                                                \
		using OwFieldOwner = std::remove_cvref_t<decltype(*(pObject))>;                                                                                       \
		static const std::size_t s_uOffset = ::OmegaWare::Unreal::ResolveFieldOffset(OwFieldOwner::StaticClass(), #Member, offsetof(OwFieldOwner, Member));          \
		return s_uOffset;                                                                                                                                      \
	}()))

#endif
