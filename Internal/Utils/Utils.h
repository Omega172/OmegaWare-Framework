#pragma once

#include <filesystem>
#include <optional>

#include "Console/Console.h"
#include "Logging/Logging.h"

namespace Utils
{
	/**
	 * Returns the path to the log file as requested, if sFile is empty then the directory path will be returned.
	 */
	std::optional<std::filesystem::path> GetLogFilePath(const std::string& sFile = "", const std::string& sExtension = "");

	/**
	 * Returns the path to the config file as requested, if sFile is empty then the directory path will be returned.
	 */
	std::optional<std::filesystem::path> GetConfigFilePath(const std::string& sFile = "", const std::string& sExtension = "");

	/**
	 * Returns the path to the lua file as requested, if sFile is empty then the directory path will be returned.
	 */
	std::optional<std::filesystem::path> GetLuaFilePath(const std::string& sFile = "", const std::string& sExtension = "lua");
	
	inline std::mt19937_64& GetRandomEngine() 
	{
		static std::random_device device{};
		static std::mt19937_64 engine(device());
		return engine;
	};

	/**
	 * Returns a secure random integral value between the minimum and maximum limits.
	 */
	template<typename T>
	std::enable_if_t<std::is_integral_v<T>, T> Random(T tMin = std::numeric_limits<T>::min, T tMax = std::numeric_limits<T>::max)
	{
		using dist_t = std::uniform_int_distribution<T>;
		static dist_t dist{ tMin, tMax };

		if (dist.min() != tMin || dist.max() != tMax)
			dist.param(dist_t::param_type(tMin, tMax));

		return dist(GetRandomEngine());
	};

	/**
	 * Returns a secure random floating point value between the minimum and maximum limits.
	 */
	template<typename T>
	std::enable_if_t<std::is_floating_point_v<T>, T> Random(T tMin = std::numeric_limits<T>::min, T tMax = std::numeric_limits<T>::max)
	{
		using dist_t = std::uniform_real_distribution<T>;
		static dist_t dist{ tMin, tMax };

		if (dist.min != tMin || dist.max != tMax)
			dist.param(dist_t::param_type(tMin, tMax));

		return dist(GetRandomEngine());
	};
}