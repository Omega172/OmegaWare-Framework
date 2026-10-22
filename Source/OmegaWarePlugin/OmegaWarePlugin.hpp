#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

namespace OmegaWare::Plugin
{
	inline constexpr std::uint32_t kApiVersion = 1;

	enum class ELogLevel : std::uint32_t
	{
		Debug,
		Info,
		Warning,
		Error,
	};

	enum class EEngine : std::uint32_t
	{
		None,
		Unreal,
		Unity,
	};

	enum class EDepthMode : std::uint32_t
	{
		Visible,
		XRay,
		Always,
		WriteDepth,
	};

	enum class EWindowFlags : std::uint32_t
	{
		None = 0,
		NoTitleBar = 1u << 0,
		NoResize = 1u << 1,
		NoMove = 1u << 2,
		NoScrollbar = 1u << 3,
		NoScrollWithMouse = 1u << 4,
		NoCollapse = 1u << 5,
		NoMaximize = 1u << 6,
		NoBackground = 1u << 7,
		NoBringToFront = 1u << 8,
		AlwaysAutoResize = 1u << 9,
		NoSavedSettings = 1u << 10,
		NoNav = 1u << 11,
		NoInputs = 1u << 12,
		AlwaysVisible = 1u << 15,
		NoSnap = 1u << 16,
	};

	[[nodiscard]] constexpr EWindowFlags operator|(EWindowFlags eLeft, EWindowFlags eRight)
	{
		return static_cast<EWindowFlags>(static_cast<std::uint32_t>(eLeft) | static_cast<std::uint32_t>(eRight));
	}

	struct stFeatureDesc
	{
		const char* szName = nullptr;
		void* pContext = nullptr;
		bool (*pfnSetup)(void* pContext) = nullptr;
		void (*pfnDestroy)(void* pContext) = nullptr;
		void (*pfnRun)(void* pContext) = nullptr;
		void (*pfnRender)(void* pContext) = nullptr;
		void (*pfnDrawMenu)(void* pContext) = nullptr;
	};

	using FnProcessEventBefore = void (*)(void* pContext, void* pObject, void* pFunction, void* pParams, bool* pbSkipOriginal);
	using FnProcessEventAfter = void (*)(void* pContext, void* pObject, void* pFunction, void* pParams);

	struct stApi
	{
		std::uint32_t uVersion = kApiVersion;
		std::uint32_t uSize = sizeof(stApi);
		void* pHost = nullptr;
		EEngine eEngine = EEngine::None;
		const char* szGame = nullptr;
		const char* szDataDirectory = nullptr;

		void (*Log)(void* pHost, ELogLevel eLevel, const char* szMessage) = nullptr;
		bool (*RegisterFeature)(void* pHost, const stFeatureDesc* pDesc) = nullptr;

		bool* (*ConfigBool)(const char* szKey, bool bDefault) = nullptr;
		int* (*ConfigInt)(const char* szKey, int iDefault) = nullptr;
		float* (*ConfigFloat)(const char* szKey, float flDefault) = nullptr;

		bool (*BeginSection)(const char* szLabel) = nullptr;
		void (*EndSection)() = nullptr;
		void (*BeginColumns)(int iColumns) = nullptr;
		void (*NextColumn)() = nullptr;
		void (*EndColumns)() = nullptr;
		void (*Text)(const char* szText) = nullptr;
		void (*TextMuted)(const char* szText) = nullptr;
		bool (*Checkbox)(const char* szLabel, bool* pValue) = nullptr;
		bool (*Toggle)(const char* szLabel, bool* pValue) = nullptr;
		bool (*SliderInt)(const char* szLabel, int* pValue, int iMin, int iMax) = nullptr;
		bool (*SliderFloat)(const char* szLabel, float* pValue, float flMin, float flMax, const char* szFormat) = nullptr;
		bool (*Button)(const char* szLabel) = nullptr;
		void (*SameLine)() = nullptr;
		void (*Separator)() = nullptr;

		void (*GetDisplaySize)(float* pflWidth, float* pflHeight) = nullptr;
		std::uint32_t (*AccentColor)() = nullptr;
		void (*DrawLine)(float flX1, float flY1, float flX2, float flY2, std::uint32_t uColor, float flThickness) = nullptr;
		void (*DrawRect)(float flX1, float flY1, float flX2, float flY2, std::uint32_t uColor, float flThickness) = nullptr;
		void (*DrawRectFilled)(float flX1, float flY1, float flX2, float flY2, std::uint32_t uColor) = nullptr;
		void (*DrawCircle)(float flX, float flY, float flRadius, std::uint32_t uColor, float flThickness) = nullptr;
		void (*DrawText)(float flX, float flY, std::uint32_t uColor, const char* szText) = nullptr;

		void* (*HookCreate)(void* pHost, void* pTarget, void* pDetour, void** ppOriginal) = nullptr;
		bool (*HookRemove)(void* pHost, void* pHook) = nullptr;
		void (*CallEnter)(void* pHost) = nullptr;
		void (*CallLeave)(void* pHost) = nullptr;

		std::uint32_t (*OnProcessEvent)(void* pHost, const char* szFunction, FnProcessEventBefore pfnBefore, FnProcessEventAfter pfnAfter, void* pContext) = nullptr;
		bool (*RemoveProcessEvent)(void* pHost, std::uint32_t uHandle) = nullptr;

		std::uint32_t* (*ConfigColor)(const char* szKey, std::uint32_t uDefault) = nullptr;
		const char* (*ConfigString)(const char* szKey, const char* szDefault) = nullptr;
		void (*SetConfigString)(const char* szKey, const char* szValue) = nullptr;
		void* (*ConfigKeybind)(const char* szKey, int iDefaultKey) = nullptr;
		bool (*KeybindActive)(const void* pKeybind) = nullptr;

		bool (*Combo)(const char* szLabel, int* pCurrent, const char* const* aszItems, int iCount) = nullptr;
		bool (*ColorEdit)(const char* szLabel, std::uint32_t* pColor, bool bAlpha) = nullptr;
		bool (*Hotkey)(const char* szLabel, int* pKey) = nullptr;
		bool (*Keybind)(const char* szLabel, void* pKeybind) = nullptr;
		bool (*InputText)(const char* szLabel, char* szBuffer, std::uint32_t uSize) = nullptr;

		void (*WorldSetCamera)(const float* pViewProjection, float flWidth, float flHeight, bool bClipSpaceYDown) = nullptr;
		bool (*WorldToScreen)(const float* pWorld, float* pScreen) = nullptr;
		void (*WorldLine)(const float* pFrom, const float* pTo, std::uint32_t uColor, EDepthMode eMode, float flThickness) = nullptr;
		void (*WorldBox)(const float* pMin, const float* pMax, std::uint32_t uColor, bool bFilled, EDepthMode eMode) = nullptr;
		void (*WorldCircle)(const float* pCenter, const float* pNormal, float flRadius, std::uint32_t uColor, EDepthMode eMode) = nullptr;
		void (*WorldSphere)(const float* pCenter, float flRadius, std::uint32_t uColor, EDepthMode eMode) = nullptr;
		void (*WorldText)(const float* pAnchor, const char* szText, std::uint32_t uColor, EDepthMode eMode) = nullptr;

		void* (*UnrealWorld)() = nullptr;
		void* (*UnrealPlayerController)() = nullptr;
		void* (*UnrealLocalPawn)() = nullptr;
		void* (*UnrealFindObject)(const char* szFullName) = nullptr;
		bool (*UnrealObjectName)(const void* pObject, char* szBuffer, std::uint32_t uSize) = nullptr;
		bool (*UnrealWorldToScreen)(const double* pWorld, float* pScreen) = nullptr;
		bool (*UnrealCamera)(double* pLocation, double* pRotation, float* pflFov) = nullptr;

		bool (*RunOnGameThread)(void* pHost, void (*pfnTask)(void* pContext), void* pContext) = nullptr;
		void* (*UnrealObjectArray)() = nullptr;
		void* (*UnrealAppendString)() = nullptr;
		bool (*Selectable)(const char* szLabel, bool bSelected) = nullptr;

		void* (*ImageCreate)(void* pHost, const std::uint8_t* pPixelsRGBA, int iWidth, int iHeight) = nullptr;
		void (*ImageDestroy)(void* pHost, void* pImage) = nullptr;
		void (*Image)(void* pImage, float flWidth, float flHeight, std::uint32_t uTint) = nullptr;
		void (*DrawImage)(void* pImage, float flX1, float flY1, float flX2, float flY2, std::uint32_t uTint) = nullptr;
		void (*ProgressBar)(float flFraction, const char* szOverlay, std::uint32_t uColor) = nullptr;
		void (*TextColored)(std::uint32_t uColor, const char* szText) = nullptr;
		void (*BeginGroup)() = nullptr;
		void (*EndGroup)() = nullptr;

		bool (*BeginWindow)(const char* szName, bool* pOpen, EWindowFlags eFlags) = nullptr;
		void (*EndWindow)() = nullptr;
		void (*SetNextWindowPos)(float flX, float flY, bool bAlways) = nullptr;
		void (*SetNextWindowSize)(float flWidth, float flHeight, bool bAlways) = nullptr;

		void (*SetNextWindowSizeConstraints)(float flMinWidth, float flMinHeight, float flMaxWidth, float flMaxHeight) = nullptr;
		void (*SetNextWindowBgAlpha)(float flAlpha) = nullptr;
		void (*GetWindowSize)(float* pflWidth, float* pflHeight) = nullptr;
		void (*GetContentRegionAvail)(float* pflWidth, float* pflHeight) = nullptr;
		void (*CalcTextSize)(const char* szText, float* pflWidth, float* pflHeight) = nullptr;
		void (*GetItemSpacing)(float* pflX, float* pflY) = nullptr;
		float (*GetFrameHeight)() = nullptr;
		void (*Dummy)(float flWidth, float flHeight) = nullptr;
	};

	using FnPluginLoad = bool (*)(const stApi* pApi);
	using FnPluginUnload = void (*)();

	inline constexpr const char* kLoadExport = "OmegaWarePluginLoad";
	inline constexpr const char* kUnloadExport = "OmegaWarePluginUnload";

	[[nodiscard]] constexpr std::uint32_t Rgba(std::uint8_t byRed, std::uint8_t byGreen, std::uint8_t byBlue, std::uint8_t byAlpha = 255)
	{
		return static_cast<std::uint32_t>(byRed) | (static_cast<std::uint32_t>(byGreen) << 8) | (static_cast<std::uint32_t>(byBlue) << 16) | (static_cast<std::uint32_t>(byAlpha) << 24);
	}

#if !OMEGAWARE_PLUGIN_HOST
	inline const stApi* g_pApi = nullptr;

	[[nodiscard]] inline const stApi& Api()
	{
		return *g_pApi;
	}

	inline void Log(ELogLevel eLevel, std::string_view strMessage)
	{
		Api().Log(Api().pHost, eLevel, std::string(strMessage).c_str());
	}

	class clCallGuard
	{
	public:
		clCallGuard() { Api().CallEnter(Api().pHost); }
		~clCallGuard() { Api().CallLeave(Api().pHost); }

		clCallGuard(const clCallGuard&) = delete;
		clCallGuard& operator=(const clCallGuard&) = delete;
	};

	class clFeature
	{
	public:
		virtual ~clFeature() = default;
		[[nodiscard]] virtual const char* GetName() const = 0;
		virtual bool Setup() { return true; }
		virtual void Destroy() {}
		virtual void Run() {}
		virtual void Render() {}
		virtual void DrawMenu() {}
	};

	inline bool RegisterFeature(clFeature& rFeature)
	{
		stFeatureDesc stDesc;
		stDesc.szName = rFeature.GetName();
		stDesc.pContext = &rFeature;
		stDesc.pfnSetup = [](void* pContext) { return static_cast<clFeature*>(pContext)->Setup(); };
		stDesc.pfnDestroy = [](void* pContext) { static_cast<clFeature*>(pContext)->Destroy(); };
		stDesc.pfnRun = [](void* pContext) { static_cast<clFeature*>(pContext)->Run(); };
		stDesc.pfnRender = [](void* pContext) { static_cast<clFeature*>(pContext)->Render(); };
		stDesc.pfnDrawMenu = [](void* pContext) { static_cast<clFeature*>(pContext)->DrawMenu(); };
		return Api().RegisterFeature(Api().pHost, &stDesc);
	}

	template <typename Fn>
	class clHook
	{
	public:
		~clHook() { Remove(); }

		bool Install(void* pTarget, Fn pfnDetour)
		{
			if (m_pHook)
				return false;
			void* pOriginal = nullptr;
			m_pHook = Api().HookCreate(Api().pHost, pTarget, reinterpret_cast<void*>(pfnDetour), &pOriginal);
			m_pfnOriginal = reinterpret_cast<Fn>(pOriginal);
			return m_pHook != nullptr;
		}

		bool Remove()
		{
			if (!m_pHook || !g_pApi)
				return true;
			const bool bRemoved = Api().HookRemove(Api().pHost, m_pHook);
			m_pHook = nullptr;
			return bRemoved;
		}

		template <typename... Args>
		decltype(auto) operator()(Args&&... args) const
		{
			return m_pfnOriginal(std::forward<Args>(args)...);
		}

		[[nodiscard]] bool IsInstalled() const { return m_pHook != nullptr; }

	private:
		void* m_pHook = nullptr;
		Fn m_pfnOriginal = nullptr;
	};
#endif
}

#if !OMEGAWARE_PLUGIN_HOST
#define OMEGAWARE_PLUGIN(fnLoad, fnUnload) \
	extern "C" __declspec(dllexport) bool OmegaWarePluginLoad(const ::OmegaWare::Plugin::stApi* pApi) \
	{ \
		if (!pApi || pApi->uVersion != ::OmegaWare::Plugin::kApiVersion || pApi->uSize < sizeof(::OmegaWare::Plugin::stApi)) \
			return false; \
		::OmegaWare::Plugin::g_pApi = pApi; \
		return fnLoad(); \
	} \
	extern "C" __declspec(dllexport) void OmegaWarePluginUnload() \
	{ \
		fnUnload(); \
		::OmegaWare::Plugin::g_pApi = nullptr; \
	}
#endif
