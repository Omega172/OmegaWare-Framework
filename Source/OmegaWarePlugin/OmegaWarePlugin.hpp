#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

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

	struct stExtensionDesc
	{
		const char* szSlot = nullptr;
		void* pContext = nullptr;
		void (*pfnDraw)(void* pContext, void* pArgument) = nullptr;
		std::int32_t iOrder = 0;
	};

	enum class EStyleColor : std::uint32_t
	{
		Background,
		Surface,
		Surface2,
		Hover,
		Text,
		TextMuted,
		TextDisabled,
		TextHeading,
		Border,
		BorderHover,
		Accent,
		AccentActive,
		AccentText,
		AccentSoft,
		AccentGlow,
		Danger,
		DangerSoft,
		Success,
		SuccessSoft,
		Warning,
		DotGrid,
		Shadow,
		ModalDim,
		Selection,
		ControlClose,
		ControlMinimize,
		ControlMaximize,
		Count,
	};

	using FnThemeApply = void (*)(float flDeltaTime);

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

		std::size_t (*UnrealFieldOffset)(void* pClass, const char* szMember, std::size_t uSdkOffset) = nullptr;

		bool (*RegisterExtension)(void* pHost, const stExtensionDesc* pDesc) = nullptr;
		void (*DrawExtensions)(const char* szSlot, void* pArgument) = nullptr;
		std::uint32_t (*ExtensionCount)(const char* szSlot) = nullptr;
		std::uint32_t (*FeatureCount)() = nullptr;
		bool (*FeatureInfo)(std::uint32_t uIndex, char* szName, std::uint32_t uSize, bool* pbActive) = nullptr;

		bool (*SmallButton)(const char* szLabel) = nullptr;
		void (*PushID)(const char* szID) = nullptr;
		void (*PopID)() = nullptr;
		void (*BeginDisabled)(bool bDisabled) = nullptr;
		void (*EndDisabled)() = nullptr;
		void (*Indent)() = nullptr;
		void (*Unindent)() = nullptr;
		void (*Spacing)() = nullptr;
		void (*TextWrapped)(const char* szText) = nullptr;
		void (*Tooltip)(const char* szText) = nullptr;
		bool (*BeginMenu)(const char* szLabel, bool bEnabled) = nullptr;
		void (*EndMenu)() = nullptr;
		bool (*MenuItem)(const char* szLabel, const char* szShortcut, bool* pSelected, bool bEnabled) = nullptr;
		void (*RegisterTheme)(const char* szName, FnThemeApply pfnApply) = nullptr;
		void (*UnregisterTheme)(const char* szName) = nullptr;
		std::uint32_t (*GetStyleColor)(EStyleColor eColor) = nullptr;
		void (*SetStyleColor)(EStyleColor eColor, std::uint32_t uRgba) = nullptr;
		void (*SetStyleAccent)(std::uint32_t uRgba) = nullptr;
		void (*PushToast)(std::uint32_t uKind, const char* szText) = nullptr;

		bool (*MemModule)(const char* szName, std::uintptr_t* puBase, std::size_t* puSize) = nullptr;
		std::uint32_t (*MemModuleCount)() = nullptr;
		bool (*MemModuleAt)(std::uint32_t uIndex, char* szName, std::uint32_t uNameSize, std::uintptr_t* puBase, std::size_t* puSize) = nullptr;
		std::uintptr_t (*MemFindPattern)(const char* szModule, const char* szPattern) = nullptr;
		std::uint32_t (*MemFindPatterns)(const char* szModule, const char* szPattern, std::uintptr_t* puOut, std::uint32_t uCapacity) = nullptr;
		std::uintptr_t (*MemResolveRelative)(std::uintptr_t uInstruction, std::size_t uDisplacementOffset, std::size_t uInstructionLength) = nullptr;
		std::uintptr_t (*MemExport)(const char* szModule, const char* szSymbol) = nullptr;
		bool (*MemRead)(std::uintptr_t uAddress, void* pOut, std::size_t uLength) = nullptr;
		bool (*MemWrite)(std::uintptr_t uAddress, const void* pBytes, std::size_t uLength) = nullptr;
		bool (*MemIsReadable)(const void* pAddress, std::size_t uLength) = nullptr;
		bool (*MemIsWritable)(const void* pAddress, std::size_t uLength) = nullptr;
		bool (*MemIsExecutable)(const void* pAddress) = nullptr;
		void* (*MemPatch)(void* pHost, std::uintptr_t uAddress, const void* pBytes, std::size_t uLength) = nullptr;
		bool (*MemUnpatch)(void* pHost, void* pPatch) = nullptr;
		void* (*MemAlloc)(void* pHost, std::size_t uSize, bool bExecutable) = nullptr;
		bool (*MemFree)(void* pHost, void* pBlock) = nullptr;
		void* (*MemVirtualMethod)(const void* pObject, std::size_t uIndex) = nullptr;
		void* (*HookVirtual)(void* pHost, const void* pObject, std::size_t uIndex, void* pDetour, void** ppOriginal) = nullptr;

		bool (*EngineReady)() = nullptr;
		bool (*UnrealIsGameThread)() = nullptr;
		void* (*UnrealGameInstance)() = nullptr;
		void* (*UnrealGameState)() = nullptr;
		void* (*UnrealLocalPlayer)() = nullptr;
		void* (*UnrealViewport)() = nullptr;
		void* (*UnrealCameraManager)() = nullptr;
		void* (*UnrealFindClass)(const char* szName) = nullptr;
		void* (*UnrealObjectClass)(const void* pObject) = nullptr;
		void* (*UnrealObjectOuter)(const void* pObject) = nullptr;
		bool (*UnrealIsA)(const void* pObject, const void* pClass) = nullptr;
		std::uint32_t (*UnrealObjectCount)() = nullptr;
		void* (*UnrealObjectAt)(std::uint32_t uIndex) = nullptr;
		std::size_t (*UnrealPropertyOffset)(void* pStruct, const char* szMember) = nullptr;
		std::size_t (*UnrealPropertySize)(void* pStruct, const char* szMember) = nullptr;
		bool (*UnrealGetProperty)(const void* pObject, const char* szMember, void* pOut, std::size_t uSize) = nullptr;
		bool (*UnrealSetProperty)(void* pObject, const char* szMember, const void* pValue, std::size_t uSize) = nullptr;
		void* (*UnrealFindFunction)(void* pClass, const char* szFunction) = nullptr;
		std::size_t (*UnrealFunctionParamsSize)(const void* pFunction) = nullptr;
		std::size_t (*UnrealFunctionParamOffset)(void* pFunction, const char* szParam) = nullptr;
		bool (*UnrealCall)(void* pObject, void* pFunction, void* pParams) = nullptr;

		bool (*UnityAvailable)() = nullptr;
		void* (*UnityGetClass)(const char* szNamespace, const char* szName, const char* szAssembly) = nullptr;
		void* (*UnityGetMethod)(void* pClass, const char* szName, int iParameterCount) = nullptr;
		void* (*UnityMethodPointer)(const void* pMethod) = nullptr;
		void* (*UnityCompiledMethod)(const char* szNamespace, const char* szClass, const char* szMethod, int iParameterCount, const char* szAssembly) = nullptr;
		void* (*UnityGetField)(void* pClass, const char* szName) = nullptr;
		std::size_t (*UnityFieldOffset)(void* pField) = nullptr;
		bool (*UnityStaticGet)(void* pClass, void* pField, void* pOut) = nullptr;
		bool (*UnityStaticSet)(void* pClass, void* pField, void* pValue) = nullptr;
		bool (*UnityFieldGet)(void* pObject, void* pField, void* pOut) = nullptr;
		bool (*UnityFieldSet)(void* pObject, void* pField, void* pValue) = nullptr;
		void* (*UnityInvoke)(const void* pMethod, void* pInstance, void** ppArguments) = nullptr;
		void* (*UnityNewString)(const char* szText) = nullptr;
		bool (*UnityToString)(void* pString, char* szBuffer, std::uint32_t uSize) = nullptr;
		bool (*UnityAttachThread)() = nullptr;

		bool (*LocAddLocale)(const char* szCode, const char* szName) = nullptr;
		void (*LocAddString)(const char* szCode, const char* szKey, const char* szText) = nullptr;
		bool (*LocLoad)(const char* szContents) = nullptr;
		const char* (*LocGet)(const char* szKey) = nullptr;
		const char* (*LocLabel)(const char* szKey) = nullptr;
		bool (*LocHas)(const char* szKey) = nullptr;
		const char* (*LocLocale)() = nullptr;
		bool (*LocSetLocale)(const char* szCode) = nullptr;

		bool (*ConfigSave)(const char* szProfile) = nullptr;
		bool (*ConfigLoad)(const char* szProfile) = nullptr;
		bool (*ConfigDelete)(const char* szProfile) = nullptr;
		std::uint32_t (*ConfigProfileCount)() = nullptr;
		bool (*ConfigProfileName)(std::uint32_t uIndex, char* szName, std::uint32_t uSize) = nullptr;
		bool (*ConfigCurrent)(char* szName, std::uint32_t uSize) = nullptr;
		void (*ConfigReset)() = nullptr;
		void (*ConfigSetParent)(const char* szKey, const char* szParentKey) = nullptr;

		bool (*KeyDown)(int iVirtualKey) = nullptr;
		bool (*KeyPressed)(int iVirtualKey) = nullptr;
		bool (*MouseDown)(int iButton) = nullptr;
		void (*MousePosition)(float* pflX, float* pflY) = nullptr;
		bool (*WantsMouse)() = nullptr;
		bool (*WantsKeyboard)() = nullptr;
		bool (*MenuVisible)() = nullptr;
		void (*SetMenuVisible)(bool bVisible) = nullptr;
		bool (*ConsoleVisible)() = nullptr;
		void (*SetConsoleVisible)(bool bVisible) = nullptr;
		double (*GetTime)() = nullptr;
		float (*GetDeltaTime)() = nullptr;
		int (*GetFrameCount)() = nullptr;

		bool (*FeatureDrawMenu)(std::uint32_t uIndex) = nullptr;
		std::uint32_t (*PluginCount)() = nullptr;
		bool (*PluginInfo)(std::uint32_t uIndex, char* szName, std::uint32_t uSize, bool* pbLoaded) = nullptr;
		bool (*PluginRequestLoad)(const char* szName) = nullptr;
		bool (*PluginRequestUnload)(const char* szName) = nullptr;
		void* (*PluginExport)(const char* szPlugin, const char* szSymbol) = nullptr;

		bool (*BeginTabBar)(const char* szID) = nullptr;
		void (*EndTabBar)() = nullptr;
		bool (*BeginTabItem)(const char* szLabel, bool* pOpen) = nullptr;
		void (*EndTabItem)() = nullptr;
		void (*SetTabItemSelected)(const char* szTabBar, const char* szLabel) = nullptr;
		bool (*BeginTable)(const char* szID, int iColumns, std::uint32_t uFlags) = nullptr;
		void (*EndTable)() = nullptr;
		void (*TableSetupColumn)(const char* szLabel, std::uint32_t uFlags, float flWidthOrWeight) = nullptr;
		void (*TableHeadersRow)() = nullptr;
		void (*TableNextRow)() = nullptr;
		bool (*TableNextColumn)() = nullptr;
		bool (*TableSetColumnIndex)(int iColumn) = nullptr;
		bool (*TreeNode)(const char* szLabel, bool bDefaultOpen) = nullptr;
		void (*TreePop)() = nullptr;
		bool (*CollapsingHeader)(const char* szLabel, bool bDefaultOpen) = nullptr;
		void (*OpenPopup)(const char* szID) = nullptr;
		bool (*BeginPopup)(const char* szID) = nullptr;
		bool (*BeginPopupModal)(const char* szName, bool* pOpen) = nullptr;
		bool (*BeginPopupContextItem)(const char* szID) = nullptr;
		void (*EndPopup)() = nullptr;
		void (*CloseCurrentPopup)() = nullptr;
		bool (*BeginCombo)(const char* szLabel, const char* szPreview) = nullptr;
		void (*EndCombo)() = nullptr;
		bool (*BeginChild)(const char* szID, float flWidth, float flHeight, bool bBorder) = nullptr;
		void (*EndChild)() = nullptr;
		bool (*BeginMenuBar)() = nullptr;
		void (*EndMenuBar)() = nullptr;
		bool (*RadioButton)(const char* szLabel, int* pValue, int iButtonValue) = nullptr;
		bool (*InputInt)(const char* szLabel, int* pValue, int iStep, int iMin, int iMax) = nullptr;
		bool (*InputFloat)(const char* szLabel, float* pValue, float flStep, const char* szFormat) = nullptr;
		bool (*InputTextHint)(const char* szLabel, char* szBuffer, std::uint32_t uSize, const char* szHint) = nullptr;
		void (*PlotLines)(const char* szLabel, const float* pValues, int iCount, float flMin, float flMax, float flWidth, float flHeight) = nullptr;
		void (*PlotHistogram)(const char* szLabel, const float* pValues, int iCount, float flMin, float flMax, float flWidth, float flHeight) = nullptr;
		void (*LabelText)(const char* szLabel, const char* szValue) = nullptr;
		void (*BulletText)(const char* szText) = nullptr;
		void (*TextDisabled)(const char* szText) = nullptr;
		void (*SeparatorText)(const char* szLabel) = nullptr;
		void (*Title)(const char* szText) = nullptr;
		void (*Header)(const char* szText) = nullptr;
		void (*SubHeader)(const char* szText) = nullptr;
		bool (*TextLink)(const char* szLabel) = nullptr;
		bool (*InvisibleButton)(const char* szID, float flWidth, float flHeight) = nullptr;
		void (*NewLine)() = nullptr;
		void (*PushItemWidth)(float flWidth) = nullptr;
		void (*PopItemWidth)() = nullptr;
		void (*SetNextItemWidth)(float flWidth) = nullptr;
		void (*GetCursorPos)(float* pflX, float* pflY) = nullptr;
		void (*SetCursorPos)(float flX, float flY) = nullptr;
		void (*GetCursorScreenPos)(float* pflX, float* pflY) = nullptr;
		void (*SetCursorScreenPos)(float flX, float flY) = nullptr;
		float (*GetScrollY)() = nullptr;
		float (*GetScrollMaxY)() = nullptr;
		void (*SetScrollY)(float flScroll) = nullptr;
		bool (*IsItemHovered)() = nullptr;
		bool (*IsItemActive)() = nullptr;
		bool (*IsItemClicked)() = nullptr;
		bool (*IsItemFocused)() = nullptr;
		bool (*IsItemEdited)() = nullptr;
		void (*GetItemRect)(float* pflMinX, float* pflMinY, float* pflMaxX, float* pflMaxY) = nullptr;
		void (*GetWindowPos)(float* pflX, float* pflY) = nullptr;
		bool (*IsWindowFocused)() = nullptr;
		bool (*IsWindowHovered)() = nullptr;
		void (*SetNextWindowFocus)() = nullptr;
		void (*SetNextWindowCollapsed)(bool bCollapsed) = nullptr;
		void (*SetNextWindowOpacity)(float flOpacity) = nullptr;
		void (*SetClipboardText)(const char* szText) = nullptr;
		const char* (*GetClipboardText)() = nullptr;
		void (*SetKeyboardFocusHere)() = nullptr;
		void (*SetTooltip)(const char* szText) = nullptr;
		void (*Alert)(std::uint32_t uKind, const char* szText) = nullptr;
		void (*DrawCircleFilled)(float flX, float flY, float flRadius, std::uint32_t uColor) = nullptr;
		void (*DrawTriangle)(float flX1, float flY1, float flX2, float flY2, float flX3, float flY3, std::uint32_t uColor, float flThickness, bool bFilled) = nullptr;
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

	[[nodiscard]] inline std::size_t FieldOffset(void* pClass, const char* szMember, std::size_t uSdkOffset)
	{
		const stApi& rApi = Api();
		return rApi.UnrealFieldOffset ? rApi.UnrealFieldOffset(pClass, szMember, uSdkOffset) : uSdkOffset;
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

	inline bool Extend(const char* szSlot, void* pContext, void (*pfnDraw)(void* pContext, void* pArgument), std::int32_t iOrder = 0)
	{
		if (!Api().RegisterExtension)
			return false;
		stExtensionDesc stDesc;
		stDesc.szSlot = szSlot;
		stDesc.pContext = pContext;
		stDesc.pfnDraw = pfnDraw;
		stDesc.iOrder = iOrder;
		return Api().RegisterExtension(Api().pHost, &stDesc);
	}

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

	namespace Mem
	{
		[[nodiscard]] inline bool Module(std::string_view strName, std::uintptr_t& ruBase, std::size_t& ruSize)
		{
			const stApi& rApi = Api();
			return rApi.MemModule && rApi.MemModule(strName.empty() ? nullptr : std::string(strName).c_str(), &ruBase, &ruSize);
		}

		[[nodiscard]] inline std::uintptr_t Scan(std::string_view strModule, std::string_view strPattern)
		{
			const stApi& rApi = Api();
			return rApi.MemFindPattern ? rApi.MemFindPattern(strModule.empty() ? nullptr : std::string(strModule).c_str(), std::string(strPattern).c_str()) : 0;
		}

		[[nodiscard]] inline std::vector<std::uintptr_t> ScanAll(std::string_view strModule, std::string_view strPattern, std::uint32_t uLimit = 64)
		{
			const stApi& rApi = Api();
			std::vector<std::uintptr_t> aMatches(uLimit);
			if (!rApi.MemFindPatterns || !uLimit)
				return {};
			const std::uint32_t uCount = rApi.MemFindPatterns(strModule.empty() ? nullptr : std::string(strModule).c_str(), std::string(strPattern).c_str(), aMatches.data(), uLimit);
			aMatches.resize((std::min)(uCount, uLimit));
			return aMatches;
		}

		[[nodiscard]] inline std::uintptr_t ResolveRelative(std::uintptr_t uInstruction, std::size_t uDisplacementOffset, std::size_t uInstructionLength)
		{
			const stApi& rApi = Api();
			return rApi.MemResolveRelative ? rApi.MemResolveRelative(uInstruction, uDisplacementOffset, uInstructionLength) : 0;
		}

		[[nodiscard]] inline std::uintptr_t Export(std::string_view strModule, std::string_view strSymbol)
		{
			const stApi& rApi = Api();
			return rApi.MemExport ? rApi.MemExport(std::string(strModule).c_str(), std::string(strSymbol).c_str()) : 0;
		}

		template <typename T>
			requires std::is_trivially_copyable_v<T>
		[[nodiscard]] inline std::optional<T> Read(std::uintptr_t uAddress)
		{
			T tValue{};
			const stApi& rApi = Api();
			if (!rApi.MemRead || !rApi.MemRead(uAddress, &tValue, sizeof(T)))
				return std::nullopt;
			return tValue;
		}

		template <typename T>
			requires std::is_trivially_copyable_v<T>
		inline bool Write(std::uintptr_t uAddress, const T& rValue)
		{
			const stApi& rApi = Api();
			return rApi.MemWrite && rApi.MemWrite(uAddress, &rValue, sizeof(T));
		}

		class clPatch
		{
		public:
			clPatch() = default;
			~clPatch() { Revert(); }

			clPatch(const clPatch&) = delete;
			clPatch& operator=(const clPatch&) = delete;

			bool Apply(std::uintptr_t uAddress, std::span<const std::uint8_t> aBytes)
			{
				const stApi& rApi = Api();
				if (m_pPatch || !rApi.MemPatch)
					return false;
				m_pPatch = rApi.MemPatch(rApi.pHost, uAddress, aBytes.data(), aBytes.size());
				return m_pPatch != nullptr;
			}

			bool Nop(std::uintptr_t uAddress, std::size_t uLength)
			{
				const std::vector<std::uint8_t> aBytes(uLength, 0x90);
				return Apply(uAddress, aBytes);
			}

			bool Revert()
			{
				if (!m_pPatch || !g_pApi)
					return true;
				const bool bRestored = Api().MemUnpatch(Api().pHost, m_pPatch);
				m_pPatch = nullptr;
				return bRestored;
			}

			[[nodiscard]] bool IsApplied() const { return m_pPatch != nullptr; }

		private:
			void* m_pPatch = nullptr;
		};
	}

	namespace Loc
	{
		[[nodiscard]] inline std::string Get(std::string_view strKey)
		{
			const stApi& rApi = Api();
			return rApi.LocGet ? std::string(rApi.LocGet(std::string(strKey).c_str())) : std::string(strKey);
		}

		inline bool AddLocale(std::string_view strCode, std::string_view strName)
		{
			const stApi& rApi = Api();
			return rApi.LocAddLocale && rApi.LocAddLocale(std::string(strCode).c_str(), std::string(strName).c_str());
		}

		inline void Add(std::string_view strCode, std::string_view strKey, std::string_view strText)
		{
			const stApi& rApi = Api();
			if (rApi.LocAddString)
				rApi.LocAddString(std::string(strCode).c_str(), std::string(strKey).c_str(), std::string(strText).c_str());
		}

		[[nodiscard]] inline bool Has(std::string_view strKey)
		{
			const stApi& rApi = Api();
			return rApi.LocHas && rApi.LocHas(std::string(strKey).c_str());
		}
	}

	namespace Input
	{
		[[nodiscard]] inline bool KeyDown(int iVirtualKey) { return Api().KeyDown && Api().KeyDown(iVirtualKey); }
		[[nodiscard]] inline bool KeyPressed(int iVirtualKey) { return Api().KeyPressed && Api().KeyPressed(iVirtualKey); }
		[[nodiscard]] inline bool MouseDown(int iButton) { return Api().MouseDown && Api().MouseDown(iButton); }
	}

	namespace Unreal
	{
		[[nodiscard]] inline void* FindClass(std::string_view strName)
		{
			const stApi& rApi = Api();
			return rApi.UnrealFindClass ? rApi.UnrealFindClass(std::string(strName).c_str()) : nullptr;
		}

		[[nodiscard]] inline void* FindFunction(void* pClass, std::string_view strName)
		{
			const stApi& rApi = Api();
			return rApi.UnrealFindFunction ? rApi.UnrealFindFunction(pClass, std::string(strName).c_str()) : nullptr;
		}

		template <typename T>
			requires std::is_trivially_copyable_v<T>
		[[nodiscard]] inline std::optional<T> GetProperty(const void* pObject, std::string_view strName)
		{
			T tValue{};
			const stApi& rApi = Api();
			if (!rApi.UnrealGetProperty || !rApi.UnrealGetProperty(pObject, std::string(strName).c_str(), &tValue, sizeof(T)))
				return std::nullopt;
			return tValue;
		}

		template <typename T>
			requires std::is_trivially_copyable_v<T>
		inline bool SetProperty(void* pObject, std::string_view strName, const T& rValue)
		{
			const stApi& rApi = Api();
			return rApi.UnrealSetProperty && rApi.UnrealSetProperty(pObject, std::string(strName).c_str(), &rValue, sizeof(T));
		}

		class clCall
		{
		public:
			clCall(void* pObject, std::string_view strFunction) : m_pObject(pObject)
			{
				const stApi& rApi = Api();
				if (!pObject || !rApi.UnrealObjectClass || !rApi.UnrealFindFunction)
					return;
				m_pFunction = FindFunction(rApi.UnrealObjectClass(pObject), strFunction);
				if (m_pFunction)
					m_aParams.resize(rApi.UnrealFunctionParamsSize(m_pFunction));
			}

			[[nodiscard]] bool IsValid() const { return m_pFunction != nullptr; }

			template <typename T>
				requires std::is_trivially_copyable_v<T>
			bool Set(std::string_view strParam, const T& rValue)
			{
				const std::size_t uOffset = ParamOffset(strParam, sizeof(T));
				if (uOffset == SIZE_MAX)
					return false;
				std::memcpy(m_aParams.data() + uOffset, &rValue, sizeof(T));
				return true;
			}

			template <typename T>
				requires std::is_trivially_copyable_v<T>
			[[nodiscard]] std::optional<T> Get(std::string_view strParam) const
			{
				const std::size_t uOffset = ParamOffset(strParam, sizeof(T));
				if (uOffset == SIZE_MAX)
					return std::nullopt;
				T tValue{};
				std::memcpy(&tValue, m_aParams.data() + uOffset, sizeof(T));
				return tValue;
			}

			bool Invoke()
			{
				const stApi& rApi = Api();
				return m_pFunction && rApi.UnrealCall && rApi.UnrealCall(m_pObject, m_pFunction, m_aParams.data());
			}

		private:
			[[nodiscard]] std::size_t ParamOffset(std::string_view strParam, std::size_t uSize) const
			{
				const stApi& rApi = Api();
				if (!m_pFunction || !rApi.UnrealFunctionParamOffset || !rApi.UnrealPropertySize)
					return SIZE_MAX;
				const std::string strName(strParam);
				const std::size_t uOffset = rApi.UnrealFunctionParamOffset(m_pFunction, strName.c_str());
				if (uOffset == SIZE_MAX || uOffset + uSize > m_aParams.size() || rApi.UnrealPropertySize(m_pFunction, strName.c_str()) < uSize)
					return SIZE_MAX;
				return uOffset;
			}

			void* m_pObject = nullptr;
			void* m_pFunction = nullptr;
			std::vector<std::uint8_t> m_aParams;
		};
	}

	namespace Unity
	{
		[[nodiscard]] inline void* Method(std::string_view strNamespace, std::string_view strClass, std::string_view strMethod, int iParameterCount = -1, std::string_view strAssembly = {})
		{
			const stApi& rApi = Api();
			if (!rApi.UnityCompiledMethod)
				return nullptr;
			const std::string strNs(strNamespace);
			const std::string strCls(strClass);
			const std::string strFn(strMethod);
			const std::string strAsm(strAssembly);
			return rApi.UnityCompiledMethod(strNs.c_str(), strCls.c_str(), strFn.c_str(), iParameterCount, strAsm.empty() ? nullptr : strAsm.c_str());
		}
	}
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

#define OWP_FIELD(pObject, Member) \
	(*reinterpret_cast<decltype(std::remove_cvref_t<decltype(*(pObject))>::Member)*>(reinterpret_cast<std::uint8_t*>(const_cast<std::remove_cv_t<std::remove_pointer_t<decltype(pObject)>>*>(pObject)) + [] { \
		using OwpFieldOwner = std::remove_cvref_t<decltype(*(pObject))>; \
		static const std::size_t s_uOffset = ::OmegaWare::Plugin::FieldOffset(OwpFieldOwner::StaticClass(), #Member, offsetof(OwpFieldOwner, Member)); \
		return s_uOffset; \
	}()))
#endif
