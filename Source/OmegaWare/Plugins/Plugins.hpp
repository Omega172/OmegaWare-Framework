#pragma once

#include <filesystem>
#include <functional>
#include <mutex>
#include <string>
#include <vector>

namespace OmegaWare::Plugins
{
	using FnLockGui = std::function<std::unique_lock<std::recursive_mutex>()>;

	struct stPluginInfo
	{
		std::string strName;
		std::size_t uFeatures = 0;
		std::size_t uHooks = 0;
		bool bLoaded = false;
	};

	void Initialize(const std::filesystem::path& pathDirectory, FnLockGui fnLockGui);
	void Update();
	void RequestReload();
	void Shutdown();
	[[nodiscard]] std::vector<stPluginInfo> List();
	[[nodiscard]] std::filesystem::path Directory();
}
