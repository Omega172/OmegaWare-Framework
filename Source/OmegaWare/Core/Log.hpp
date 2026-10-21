#pragma once

#include "OmegaRenderHook/Core/Log.hpp"

#include <filesystem>
#include <format>
#include <string>
#include <source_location>
#include <string_view>
#include <type_traits>
#include <utility>

namespace OmegaWare
{
	using ELogLevel = OmegaRenderHook::ELogLevel;

	template <typename... Args>
	struct stLogFormat
	{
		std::format_string<Args...> fmtMessage;
		std::source_location stLocation;

		template <typename T>
		consteval stLogFormat(const T& rFormat, std::source_location stCaller = std::source_location::current())
			: fmtMessage(rFormat), stLocation(stCaller)
		{
		}
	};

	using LogListener = void (*)(ELogLevel eLevel, std::string_view strMessage);

	void SetLogListener(LogListener pfnListener);
	void OpenLog(const std::filesystem::path& pathDirectory);
	[[nodiscard]] std::string FormatLocation(const std::source_location& rLocation);
	void CloseLog();
	void WriteLog(ELogLevel eLevel, std::string_view strMessage, const std::source_location* pLocation = nullptr);

	template <typename... Args>
	void LogDebug(stLogFormat<std::type_identity_t<Args>...> stFormat, Args&&... args)
	{
		WriteLog(ELogLevel::Debug, std::format(stFormat.fmtMessage, std::forward<Args>(args)...), &stFormat.stLocation);
	}

	template <typename... Args>
	void LogInfo(stLogFormat<std::type_identity_t<Args>...> stFormat, Args&&... args)
	{
		WriteLog(ELogLevel::Info, std::format(stFormat.fmtMessage, std::forward<Args>(args)...), &stFormat.stLocation);
	}

	template <typename... Args>
	void LogWarning(stLogFormat<std::type_identity_t<Args>...> stFormat, Args&&... args)
	{
		WriteLog(ELogLevel::Warning, std::format(stFormat.fmtMessage, std::forward<Args>(args)...), &stFormat.stLocation);
	}

	template <typename... Args>
	void LogError(stLogFormat<std::type_identity_t<Args>...> stFormat, Args&&... args)
	{
		WriteLog(ELogLevel::Error, std::format(stFormat.fmtMessage, std::forward<Args>(args)...), &stFormat.stLocation);
	}
}
