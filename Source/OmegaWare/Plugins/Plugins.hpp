#pragma once

#include <filesystem>
#include <functional>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

namespace OmegaGUI
{
	class clRenderBackend;
}

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

	enum class ECommandResult
	{
		Queued,
		NotFound,
		NoChange,
	};

	struct stSlotInfo
	{
		std::string strSlot;
		std::size_t uExtensions = 0;
	};

	void Initialize(const std::filesystem::path& pathDirectory, FnLockGui fnLockGui);
	void Update();
	void RequestReload();
	[[nodiscard]] ECommandResult RequestUnload(std::string_view strName);
	[[nodiscard]] ECommandResult RequestLoad(std::string_view strName);
	void Shutdown();
	[[nodiscard]] std::vector<stPluginInfo> List();
	[[nodiscard]] std::filesystem::path Directory();
	void SetImageBackend(OmegaGUI::clRenderBackend* pBackend);
	void FlushImages();
	void DrawExtensions(std::string_view strSlot, void* pArgument = nullptr);
	[[nodiscard]] std::size_t ExtensionCount(std::string_view strSlot);
	[[nodiscard]] std::vector<stSlotInfo> Slots();
}
