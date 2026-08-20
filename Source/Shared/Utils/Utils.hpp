#pragma once

#include <filesystem>
#include <optional>

#include "Console/Console.hpp"
#include "Logging/Logging.hpp"

namespace Utils
{
	std::optional<std::filesystem::path> GetLogFilePath(const std::string& sFile = "", const std::string& sExtension = "");

	std::optional<std::filesystem::path> GetConfigFilePath(const std::string& sFile = "", const std::string& sExtension = "");

	std::optional<std::filesystem::path> GetLuaFilePath(const std::string& sFile = "", const std::string& sExtension = "lua");

	bool OpenFolder(const std::filesystem::path& pathFolder);

	inline std::mt19937_64& GetRandomEngine() 
	{
		static std::random_device device{};
		static std::mt19937_64 engine(device());
		return engine;
	};

    // This would be a concept but MSVC doesnt seem to be doing too good with them right now.
    template<typename T>
    constexpr auto is_int_distribution_defined = std::is_same_v<T, short> || std::is_same_v<T, int> || std::is_same_v<T, long> || std::is_same_v<T, long long> ||
        std::is_same_v<T, unsigned short> || std::is_same_v<T, unsigned int> || std::is_same_v<T, unsigned long> || std::is_same_v<T, unsigned long long>;

    template<typename T>
    std::enable_if_t<std::is_integral_v<T>&& is_int_distribution_defined<T>, T> Random(T tMin = std::numeric_limits<T>::min(), T tMax = std::numeric_limits<T>::max())
    {
        using dist_t = std::uniform_int_distribution<T>;
        static dist_t dist{ tMin, tMax };

        if (dist.min() != tMin || dist.max() != tMax)
            dist.param(typename dist_t::param_type{ tMin, tMax });

        return dist(GetRandomEngine());
    };

    template<typename T>
    std::enable_if_t<std::is_integral_v<T> && !is_int_distribution_defined<T> && !std::is_same_v<T, bool>, T> Random(T tMin = std::numeric_limits<T>::min(), T tMax = std::numeric_limits<T>::max())
    {
        using dist_t = std::uniform_int_distribution<long long>;
        static dist_t dist{ static_cast<long long>(tMin), static_cast<long long>(tMax) };

        if (dist.min() != tMin || dist.max() != tMax)
            dist.param(typename dist_t::param_type{ tMin, tMax });

        return dist(GetRandomEngine());
    };

    template<typename T>
    std::enable_if_t<std::is_floating_point_v<T>, T> Random(T tMin = std::numeric_limits<T>::min(), T tMax = std::numeric_limits<T>::max())
    {
        using dist_t = std::uniform_real_distribution<T>;
        static dist_t dist{ tMin, tMax };

        if (dist.min() != tMin || dist.max() != tMax)
            dist.param(typename dist_t::param_type{ tMin, tMax });

        return dist(GetRandomEngine());
    };

    template<typename T>
    std::enable_if_t<std::is_same_v<bool, T>, T> Random(T tMin = std::numeric_limits<T>::min(), T tMax = std::numeric_limits<T>::max())
    {
        static std::uniform_int_distribution<short> dist{ static_cast<short>(0), static_cast<short>(1) };
        return dist(GetRandomEngine()) == 1;
    };
}