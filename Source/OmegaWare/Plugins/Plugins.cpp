#include "Plugins/Plugins.hpp"
#include "Config/Config.hpp"
#include "Core/Framework.hpp"
#include "Core/Log.hpp"
#include "Engines/Engine.hpp"
#include "Features/Feature.hpp"
#include "Hooks/Hook.hpp"
#include "Memory/Safety.hpp"

#include "Menu/Widgets.hpp"

#if OMEGAWARE_ENGINE_UNREAL
#include "Engines/Unreal/ProcessEvent.hpp"
#include "Engines/Unreal/Unreal.hpp"
#endif

#include "OmegaWarePlugin.hpp"

#include "OmegaGUI/OmegaGUI.hpp"
#include "OmegaRenderHook/Core/Guarded.hpp"
#include "OmegaRenderHook/Hook/InlineHook.hpp"

#include <Windows.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstring>
#include <cwctype>
#include <map>
#include <memory>
#include <set>
#include <thread>

namespace OmegaWare::Plugins
{
	namespace
	{
		namespace fs = std::filesystem;
		using namespace std::chrono_literals;

		struct stPlugin;

		class clPluginFeature final : public clFeature
		{
		public:
			clPluginFeature(const Plugin::stFeatureDesc& rDesc) : m_stDesc(rDesc), m_strName(rDesc.szName ? rDesc.szName : "plugin")
			{
				m_stDesc.szName = nullptr;
			}

			[[nodiscard]] std::string_view GetName() const override { return m_strName; }
			[[nodiscard]] std::string GetDisplayName() const override { return m_strName; }
			bool Setup() override { return !m_stDesc.pfnSetup || m_stDesc.pfnSetup(m_stDesc.pContext); }
			void Destroy() override { if (m_stDesc.pfnDestroy) m_stDesc.pfnDestroy(m_stDesc.pContext); }
			void Run() override { if (m_stDesc.pfnRun) m_stDesc.pfnRun(m_stDesc.pContext); }
			void Render() override { if (m_stDesc.pfnRender) m_stDesc.pfnRender(m_stDesc.pContext); }
			void DrawMenu() override { if (m_stDesc.pfnDrawMenu) m_stDesc.pfnDrawMenu(m_stDesc.pContext); }

		private:
			Plugin::stFeatureDesc m_stDesc;
			std::string m_strName;
		};

		struct stPlugin
		{
			fs::path pathFile;
			std::string strName;
			fs::file_time_type tpWrite{};
			std::uintmax_t uSize = 0;
			HMODULE hModule = nullptr;
			fs::path pathShadow;
			Plugin::FnPluginUnload pfnUnload = nullptr;
			Plugin::stApi stApi;
			std::vector<std::unique_ptr<clPluginFeature>> aFeatures;
			std::vector<std::unique_ptr<OmegaRenderHook::clInlineHook>> aHooks;
			std::vector<std::uint32_t> aEvents;
			std::atomic<int> iCalls = 0;
		};

		struct stObserved
		{
			fs::file_time_type tpWrite{};
			std::uintmax_t uSize = 0;
		};

		struct stState
		{
			bool bInitialized = false;
			fs::path pathDirectory;
			FnLockGui fnLockGui;
			std::map<fs::path, std::unique_ptr<stPlugin>> mapPlugins;
			std::map<fs::path, stObserved> mapPending;
			std::map<fs::path, fs::file_time_type> mapFailed;
			std::vector<std::unique_ptr<stPlugin>> aLeaked;
			std::atomic<bool> bReloadRequested = false;
			std::chrono::steady_clock::time_point tpNextScan{};
			std::uint32_t uShadowCounter = 0;
			std::string strDataDirectory;
			std::string strGame;
			std::mutex mtxSnapshot;
			std::vector<stPluginInfo> aSnapshot;
		};

		stState& State()
		{
			static stState s_stState;
			return s_stState;
		}

		[[nodiscard]] stPlugin& Owner(void* pHost)
		{
			return *static_cast<stPlugin*>(pHost);
		}

		void ApiLog(void* pHost, Plugin::ELogLevel eLevel, const char* szMessage)
		{
			const ELogLevel eHostLevel = eLevel == Plugin::ELogLevel::Debug ? ELogLevel::Debug : eLevel == Plugin::ELogLevel::Warning ? ELogLevel::Warning : eLevel == Plugin::ELogLevel::Error ? ELogLevel::Error : ELogLevel::Info;
			WriteLog(eHostLevel, std::format("[{}] {}", Owner(pHost).strName, szMessage ? szMessage : ""));
		}

		bool ApiRegisterFeature(void* pHost, const Plugin::stFeatureDesc* pDesc)
		{
			if (!pDesc)
				return false;
			stPlugin& rPlugin = Owner(pHost);
			auto pFeature = std::make_unique<clPluginFeature>(*pDesc);
			Features::Setup(*pFeature);
			rPlugin.aFeatures.push_back(std::move(pFeature));
			return true;
		}

		bool* ApiConfigBool(const char* szKey, bool bDefault) { return &Config::Add(szKey, bDefault); }
		int* ApiConfigInt(const char* szKey, int iDefault) { return &Config::Add(szKey, iDefault); }
		float* ApiConfigFloat(const char* szKey, float flDefault) { return &Config::Add(szKey, flDefault); }

		bool ApiBeginSection(const char* szLabel) { return OmegaGUI::BeginSection(szLabel); }
		void ApiEndSection() { OmegaGUI::EndSection(); }
		void ApiBeginColumns(int iColumns) { OmegaGUI::BeginColumns(iColumns); }
		void ApiNextColumn() { OmegaGUI::NextColumn(); }
		void ApiEndColumns() { OmegaGUI::EndColumns(); }
		void ApiText(const char* szText) { OmegaGUI::TextUnformatted(szText); }
		void ApiTextMuted(const char* szText) { OmegaGUI::TextMuted(szText); }
		bool ApiCheckbox(const char* szLabel, bool* pValue) { return OmegaGUI::Checkbox(szLabel, pValue); }
		bool ApiToggle(const char* szLabel, bool* pValue) { return OmegaGUI::Toggle(szLabel, pValue); }
		bool ApiSliderInt(const char* szLabel, int* pValue, int iMin, int iMax) { return OmegaGUI::SliderInt(szLabel, pValue, iMin, iMax); }
		bool ApiSliderFloat(const char* szLabel, float* pValue, float flMin, float flMax, const char* szFormat) { return OmegaGUI::SliderFloat(szLabel, pValue, flMin, flMax, szFormat ? szFormat : "{:.2f}"); }
		bool ApiButton(const char* szLabel) { return OmegaGUI::Button(szLabel); }
		void ApiSameLine() { OmegaGUI::SameLine(); }
		void ApiSeparator() { OmegaGUI::Separator(); }

		void ApiGetDisplaySize(float* pflWidth, float* pflHeight)
		{
			const OmegaGUI::vec2 vecSize = OmegaGUI::GetDisplaySize();
			if (pflWidth)
				*pflWidth = vecSize.x;
			if (pflHeight)
				*pflHeight = vecSize.y;
		}

		std::uint32_t ApiAccentColor() { return OmegaGUI::GetStyle().Get(OmegaGUI::EColor::Accent).uPacked; }
		void ApiDrawLine(float flX1, float flY1, float flX2, float flY2, std::uint32_t uColor, float flThickness) { OmegaGUI::GetBackgroundDrawList().AddLine({ flX1, flY1 }, { flX2, flY2 }, OmegaGUI::color(uColor), flThickness); }
		void ApiDrawRect(float flX1, float flY1, float flX2, float flY2, std::uint32_t uColor, float flThickness) { OmegaGUI::GetBackgroundDrawList().AddRect({ flX1, flY1 }, { flX2, flY2 }, OmegaGUI::color(uColor), 0.f, flThickness); }
		void ApiDrawRectFilled(float flX1, float flY1, float flX2, float flY2, std::uint32_t uColor) { OmegaGUI::GetBackgroundDrawList().AddRectFilled({ flX1, flY1 }, { flX2, flY2 }, OmegaGUI::color(uColor)); }
		void ApiDrawCircle(float flX, float flY, float flRadius, std::uint32_t uColor, float flThickness) { OmegaGUI::GetBackgroundDrawList().AddCircle({ flX, flY }, flRadius, OmegaGUI::color(uColor), 0, flThickness); }
		void ApiDrawText(float flX, float flY, std::uint32_t uColor, const char* szText) { OmegaGUI::GetBackgroundDrawList().AddText({ flX, flY }, OmegaGUI::color(uColor), szText ? szText : ""); }

		std::uint32_t* ApiConfigColor(const char* szKey, std::uint32_t uDefault) { return &Config::Add(szKey, OmegaGUI::color(uDefault)).uPacked; }
		const char* ApiConfigString(const char* szKey, const char* szDefault) { return Config::Add(szKey, std::string(szDefault ? szDefault : "")).c_str(); }
		void ApiSetConfigString(const char* szKey, const char* szValue) { Config::Add(szKey, std::string()) = szValue ? szValue : ""; }

		void* ApiConfigKeybind(const char* szKey, int iDefaultKey)
		{
			Config::stKeybind stDefault;
			stDefault.uKey = static_cast<OmegaGUI::KeyCode>(iDefaultKey);
			return &Config::Add(szKey, stDefault);
		}

		bool ApiKeybindActive(const void* pKeybind) { return pKeybind && static_cast<const Config::stKeybind*>(pKeybind)->bActive; }

		bool ApiCombo(const char* szLabel, int* pCurrent, const char* const* aszItems, int iCount)
		{
			if (!pCurrent || !aszItems || iCount <= 0)
				return false;
			std::vector<std::string_view> aItems(aszItems, aszItems + iCount);
			for (std::string_view& rItem : aItems)
				if (!rItem.data())
					rItem = {};
			return OmegaGUI::Combo(szLabel, pCurrent, aItems);
		}

		bool ApiColorEdit(const char* szLabel, std::uint32_t* pColor, bool bAlpha)
		{
			if (!pColor)
				return false;
			OmegaGUI::color col(*pColor);
			const bool bChanged = OmegaGUI::ColorEdit(szLabel, &col, bAlpha);
			*pColor = col.uPacked;
			return bChanged;
		}

		bool ApiHotkey(const char* szLabel, int* pKey)
		{
			if (!pKey)
				return false;
			OmegaGUI::KeyCode uKey = static_cast<OmegaGUI::KeyCode>(*pKey);
			const bool bChanged = OmegaGUI::Hotkey(szLabel, &uKey);
			*pKey = uKey;
			return bChanged;
		}

		bool ApiKeybind(const char* szLabel, void* pKeybind)
		{
			if (!pKeybind || !szLabel)
				return false;
			return Widgets::Keybind(szLabel, szLabel, *static_cast<Config::stKeybind*>(pKeybind));
		}

		bool ApiInputText(const char* szLabel, char* szBuffer, std::uint32_t uSize)
		{
			if (!szBuffer || !uSize)
				return false;
			std::string strText(szBuffer, strnlen(szBuffer, uSize));
			if (!OmegaGUI::InputText(szLabel, &strText))
				return false;
			const std::size_t uLength = std::min<std::size_t>(strText.size(), uSize - 1);
			std::memcpy(szBuffer, strText.data(), uLength);
			szBuffer[uLength] = '\0';
			return true;
		}

		[[nodiscard]] OmegaGUI::vec3 Vec3(const float* pValue)
		{
			return pValue ? OmegaGUI::vec3{ pValue[0], pValue[1], pValue[2] } : OmegaGUI::vec3{};
		}

		[[nodiscard]] OmegaGUI::EDepthMode DepthMode(Plugin::EDepthMode eMode)
		{
			switch (eMode)
			{
			case Plugin::EDepthMode::XRay: return OmegaGUI::EDepthMode::XRay;
			case Plugin::EDepthMode::Always: return OmegaGUI::EDepthMode::Always;
			case Plugin::EDepthMode::WriteDepth: return OmegaGUI::EDepthMode::WriteDepth;
			default: return OmegaGUI::EDepthMode::Visible;
			}
		}

		void ApiWorldSetCamera(const float* pViewProjection, float flWidth, float flHeight, bool bClipSpaceYDown)
		{
			if (!pViewProjection)
				return;
			OmegaGUI::mat4 matViewProjection;
			std::copy_n(pViewProjection, matViewProjection.m.size(), matViewProjection.m.begin());
			OmegaGUI::GetWorldDrawList().SetCamera(matViewProjection, { flWidth, flHeight }, bClipSpaceYDown);
		}

		bool ApiWorldToScreen(const float* pWorld, float* pScreen)
		{
			const std::optional<OmegaGUI::vec2> optScreen = pWorld ? OmegaGUI::GetWorldDrawList().WorldToScreen(Vec3(pWorld)) : std::nullopt;
			if (!optScreen || !pScreen)
				return false;
			pScreen[0] = optScreen->x;
			pScreen[1] = optScreen->y;
			return true;
		}

		void ApiWorldLine(const float* pFrom, const float* pTo, std::uint32_t uColor, Plugin::EDepthMode eMode, float flThickness)
		{
			OmegaGUI::stWorldStyle stStyle;
			if (flThickness > 0.f)
				stStyle.flThickness = flThickness;
			OmegaGUI::GetWorldDrawList().AddLine(Vec3(pFrom), Vec3(pTo), OmegaGUI::color(uColor), DepthMode(eMode), stStyle);
		}

		void ApiWorldBox(const float* pMin, const float* pMax, std::uint32_t uColor, bool bFilled, Plugin::EDepthMode eMode)
		{
			if (bFilled)
				OmegaGUI::GetWorldDrawList().AddBoxFilled(Vec3(pMin), Vec3(pMax), OmegaGUI::color(uColor), DepthMode(eMode));
			else
				OmegaGUI::GetWorldDrawList().AddBox(Vec3(pMin), Vec3(pMax), OmegaGUI::color(uColor), DepthMode(eMode));
		}

		void ApiWorldCircle(const float* pCenter, const float* pNormal, float flRadius, std::uint32_t uColor, Plugin::EDepthMode eMode)
		{
			OmegaGUI::GetWorldDrawList().AddCircle(Vec3(pCenter), pNormal ? Vec3(pNormal) : OmegaGUI::vec3{ 0.f, 1.f, 0.f }, flRadius, OmegaGUI::color(uColor), 32, DepthMode(eMode));
		}

		void ApiWorldSphere(const float* pCenter, float flRadius, std::uint32_t uColor, Plugin::EDepthMode eMode)
		{
			OmegaGUI::GetWorldDrawList().AddSphere(Vec3(pCenter), flRadius, OmegaGUI::color(uColor), 24, DepthMode(eMode));
		}

		void ApiWorldText(const float* pAnchor, const char* szText, std::uint32_t uColor, Plugin::EDepthMode eMode)
		{
			OmegaGUI::GetWorldDrawList().AddText(Vec3(pAnchor), szText ? szText : "", OmegaGUI::color(uColor), true, {}, DepthMode(eMode));
		}

#if OMEGAWARE_ENGINE_UNREAL
		void* ApiUnrealWorld() { return Unreal::GetWorld(); }
		void* ApiUnrealPlayerController() { return Unreal::GetPlayerController(); }
		void* ApiUnrealLocalPawn() { return Unreal::GetLocalPawn(); }
		void* ApiUnrealFindObject(const char* szFullName) { return szFullName && Unreal::IsResolved() ? SDK::UObject::FindObject<SDK::UObject>(szFullName) : nullptr; }

		bool ApiUnrealObjectName(const void* pObject, char* szBuffer, std::uint32_t uSize)
		{
			if (!pObject || !szBuffer || !uSize)
				return false;
			const std::string strName = static_cast<const SDK::UObject*>(pObject)->GetFullName();
			const std::size_t uLength = std::min<std::size_t>(strName.size(), uSize - 1);
			std::memcpy(szBuffer, strName.data(), uLength);
			szBuffer[uLength] = '\0';
			return true;
		}

		bool ApiUnrealWorldToScreen(const double* pWorld, float* pScreen)
		{
			if (!pWorld || !pScreen)
				return false;
			SDK::FVector stWorld{};
			stWorld.X = static_cast<decltype(stWorld.X)>(pWorld[0]);
			stWorld.Y = static_cast<decltype(stWorld.Y)>(pWorld[1]);
			stWorld.Z = static_cast<decltype(stWorld.Z)>(pWorld[2]);
			const std::optional<SDK::FVector2D> optScreen = Unreal::WorldToScreen(stWorld);
			if (!optScreen)
				return false;
			pScreen[0] = static_cast<float>(optScreen->X);
			pScreen[1] = static_cast<float>(optScreen->Y);
			return true;
		}

		bool ApiUnrealCamera(double* pLocation, double* pRotation, float* pflFov)
		{
			SDK::APlayerCameraManager* pCamera = Unreal::GetCameraManager();
			if (!pCamera)
				return false;
			if (pLocation)
			{
				const SDK::FVector stLocation = pCamera->GetCameraLocation();
				pLocation[0] = stLocation.X;
				pLocation[1] = stLocation.Y;
				pLocation[2] = stLocation.Z;
			}
			if (pRotation)
			{
				const SDK::FRotator stRotation = pCamera->GetCameraRotation();
				pRotation[0] = stRotation.Pitch;
				pRotation[1] = stRotation.Yaw;
				pRotation[2] = stRotation.Roll;
			}
			if (pflFov)
				*pflFov = pCamera->GetFOVAngle();
			return true;
		}
#endif

		void* ApiHookCreate(void* pHost, void* pTarget, void* pDetour, void** ppOriginal)
		{
			stPlugin& rPlugin = Owner(pHost);
			if (!pTarget || !pDetour || !Memory::IsExecutable(pTarget))
			{
				LogError("plugin {} tried to hook {}, which is not executable code", rPlugin.strName, pTarget);
				return nullptr;
			}
			auto pHook = std::make_unique<OmegaRenderHook::clInlineHook>();
			if (!pHook->Install(pTarget, pDetour))
			{
				LogError("plugin {} failed to hook {}", rPlugin.strName, pTarget);
				return nullptr;
			}
			if (ppOriginal)
				*ppOriginal = pHook->Original<void*>();
			void* pHandle = pHook.get();
			rPlugin.aHooks.push_back(std::move(pHook));
			return pHandle;
		}

		bool ApiHookRemove(void* pHost, void* pHook)
		{
			for (const auto& pOwned : Owner(pHost).aHooks)
				if (pOwned.get() == pHook)
					return pOwned->Disable();
			return false;
		}

		void ApiCallEnter(void* pHost) { Owner(pHost).iCalls.fetch_add(1, std::memory_order_acq_rel); }
		void ApiCallLeave(void* pHost) { Owner(pHost).iCalls.fetch_sub(1, std::memory_order_acq_rel); }

		std::uint32_t ApiOnProcessEvent([[maybe_unused]] void* pHost, [[maybe_unused]] const char* szFunction, [[maybe_unused]] Plugin::FnProcessEventBefore pfnBefore, [[maybe_unused]] Plugin::FnProcessEventAfter pfnAfter, [[maybe_unused]] void* pContext)
		{
#if OMEGAWARE_ENGINE_UNREAL
			Unreal::FnProcessEvent fnBefore;
			if (pfnBefore)
				fnBefore = [pfnBefore, pContext](Unreal::stProcessEventCall& rCall) { pfnBefore(pContext, rCall.pObject, rCall.pFunction, rCall.pParams, &rCall.bSkipOriginal); };
			Unreal::FnProcessEvent fnAfter;
			if (pfnAfter)
				fnAfter = [pfnAfter, pContext](Unreal::stProcessEventCall& rCall) { pfnAfter(pContext, rCall.pObject, rCall.pFunction, rCall.pParams); };
			const Unreal::HookHandle uHandle = Unreal::OnProcessEvent(szFunction ? szFunction : "", std::move(fnBefore), std::move(fnAfter));
			if (uHandle != Unreal::kInvalidHook)
				Owner(pHost).aEvents.push_back(uHandle);
			return uHandle;
#else
			return 0;
#endif
		}

		bool ApiRemoveProcessEvent([[maybe_unused]] void* pHost, [[maybe_unused]] std::uint32_t uHandle)
		{
#if OMEGAWARE_ENGINE_UNREAL
			std::erase(Owner(pHost).aEvents, uHandle);
			return Unreal::RemoveProcessEvent(uHandle);
#else
			return false;
#endif
		}

		void FillApi(stState& rState, stPlugin& rPlugin)
		{
			Plugin::stApi& r = rPlugin.stApi;
			r = {};
			r.pHost = &rPlugin;
			r.eEngine = Engine::kEngine == Engine::EEngine::Unreal ? Plugin::EEngine::Unreal : Engine::kEngine == Engine::EEngine::Unity ? Plugin::EEngine::Unity : Plugin::EEngine::None;
			r.szGame = rState.strGame.c_str();
			r.szDataDirectory = rState.strDataDirectory.c_str();
			r.Log = &ApiLog;
			r.RegisterFeature = &ApiRegisterFeature;
			r.ConfigBool = &ApiConfigBool;
			r.ConfigInt = &ApiConfigInt;
			r.ConfigFloat = &ApiConfigFloat;
			r.BeginSection = &ApiBeginSection;
			r.EndSection = &ApiEndSection;
			r.BeginColumns = &ApiBeginColumns;
			r.NextColumn = &ApiNextColumn;
			r.EndColumns = &ApiEndColumns;
			r.Text = &ApiText;
			r.TextMuted = &ApiTextMuted;
			r.Checkbox = &ApiCheckbox;
			r.Toggle = &ApiToggle;
			r.SliderInt = &ApiSliderInt;
			r.SliderFloat = &ApiSliderFloat;
			r.Button = &ApiButton;
			r.SameLine = &ApiSameLine;
			r.Separator = &ApiSeparator;
			r.GetDisplaySize = &ApiGetDisplaySize;
			r.AccentColor = &ApiAccentColor;
			r.DrawLine = &ApiDrawLine;
			r.DrawRect = &ApiDrawRect;
			r.DrawRectFilled = &ApiDrawRectFilled;
			r.DrawCircle = &ApiDrawCircle;
			r.DrawText = &ApiDrawText;
			r.HookCreate = &ApiHookCreate;
			r.HookRemove = &ApiHookRemove;
			r.CallEnter = &ApiCallEnter;
			r.CallLeave = &ApiCallLeave;
			r.OnProcessEvent = &ApiOnProcessEvent;
			r.RemoveProcessEvent = &ApiRemoveProcessEvent;
			r.ConfigColor = &ApiConfigColor;
			r.ConfigString = &ApiConfigString;
			r.SetConfigString = &ApiSetConfigString;
			r.ConfigKeybind = &ApiConfigKeybind;
			r.KeybindActive = &ApiKeybindActive;
			r.Combo = &ApiCombo;
			r.ColorEdit = &ApiColorEdit;
			r.Hotkey = &ApiHotkey;
			r.Keybind = &ApiKeybind;
			r.InputText = &ApiInputText;
			r.WorldSetCamera = &ApiWorldSetCamera;
			r.WorldToScreen = &ApiWorldToScreen;
			r.WorldLine = &ApiWorldLine;
			r.WorldBox = &ApiWorldBox;
			r.WorldCircle = &ApiWorldCircle;
			r.WorldSphere = &ApiWorldSphere;
			r.WorldText = &ApiWorldText;
#if OMEGAWARE_ENGINE_UNREAL
			r.UnrealWorld = &ApiUnrealWorld;
			r.UnrealPlayerController = &ApiUnrealPlayerController;
			r.UnrealLocalPawn = &ApiUnrealLocalPawn;
			r.UnrealFindObject = &ApiUnrealFindObject;
			r.UnrealObjectName = &ApiUnrealObjectName;
			r.UnrealWorldToScreen = &ApiUnrealWorldToScreen;
			r.UnrealCamera = &ApiUnrealCamera;
#endif
		}

		template <typename F>
		[[nodiscard]] bool Guarded(const stPlugin& rPlugin, std::string_view strStage, F&& fnCall)
		{
			OmegaRenderHook::stFault stFault;
			auto fnInvoke = [&] { fnCall(); };
			if (OmegaRenderHook::InvokeGuarded(fnInvoke, stFault))
				return true;
			if (stFault.bCppException)
				LogError("plugin {} threw a C++ exception in {}", rPlugin.strName, strStage);
			else
				LogError("plugin {} faulted in {} (code {:#010x} at {})", rPlugin.strName, strStage, stFault.uCode, stFault.pAddress);
			return false;
		}

		void RemoveShadow(const fs::path& pathShadow)
		{
			std::error_code errCode;
			fs::remove(pathShadow, errCode);
			fs::path pathPdb = pathShadow;
			fs::remove(pathPdb.replace_extension(".pdb"), errCode);
		}

		void Unload(stState& rState, std::unique_ptr<stPlugin> pPlugin, bool bCallUnload)
		{
			stPlugin& rPlugin = *pPlugin;
			{
				auto lckGui = rState.fnLockGui();
				for (auto itFeature = rPlugin.aFeatures.rbegin(); itFeature != rPlugin.aFeatures.rend(); ++itFeature)
					Features::Remove(**itFeature);
#if OMEGAWARE_ENGINE_UNREAL
				for (const std::uint32_t uHandle : rPlugin.aEvents)
					Unreal::RemoveProcessEvent(uHandle);
#endif
				rPlugin.aEvents.clear();
				for (const auto& pHook : rPlugin.aHooks)
					pHook->Disable();
				if (bCallUnload && rPlugin.pfnUnload)
					static_cast<void>(Guarded(rPlugin, "unload", [&] { rPlugin.pfnUnload(); }));
				rPlugin.aFeatures.clear();
			}

			const auto tpDeadline = std::chrono::steady_clock::now() + 2s;
			while (rPlugin.iCalls.load(std::memory_order_acquire) > 0 && std::chrono::steady_clock::now() < tpDeadline)
				std::this_thread::sleep_for(1ms);
			const bool bDrained = rPlugin.iCalls.load(std::memory_order_acquire) == 0 && Hooks::WaitForCalls(1s);
			if (!bDrained)
			{
				LogWarning("plugin {} still has calls in flight, keeping its code loaded", rPlugin.strName);
				rState.aLeaked.push_back(std::move(pPlugin));
				return;
			}
			for (const auto& pHook : rPlugin.aHooks)
				pHook->Uninstall();
			rPlugin.aHooks.clear();
			if (rPlugin.hModule)
				FreeLibrary(rPlugin.hModule);
			RemoveShadow(rPlugin.pathShadow);
			LogInfo("plugin {} unloaded", rPlugin.strName);
		}

		[[nodiscard]] std::unique_ptr<stPlugin> Load(stState& rState, const fs::path& pathFile, const stObserved& stFile)
		{
			auto pPlugin = std::make_unique<stPlugin>();
			stPlugin& rPlugin = *pPlugin;
			rPlugin.pathFile = pathFile;
			rPlugin.strName = pathFile.stem().string();
			rPlugin.tpWrite = stFile.tpWrite;
			rPlugin.uSize = stFile.uSize;

			std::error_code errCode;
			const fs::path pathShadowDirectory = rState.pathDirectory / ".loaded";
			fs::create_directories(pathShadowDirectory, errCode);
			rPlugin.pathShadow = pathShadowDirectory / std::format("{}.{}.dll", rPlugin.strName, ++rState.uShadowCounter);
			if (!fs::copy_file(pathFile, rPlugin.pathShadow, fs::copy_options::overwrite_existing, errCode))
			{
				LogError("plugin {} could not be copied for loading: {}", rPlugin.strName, errCode.message());
				return nullptr;
			}
			fs::path pathPdb = pathFile;
			fs::path pathShadowPdb = rPlugin.pathShadow;
			if (fs::exists(pathPdb.replace_extension(".pdb"), errCode))
				fs::copy_file(pathPdb, pathShadowPdb.replace_extension(".pdb"), fs::copy_options::overwrite_existing, errCode);

			rPlugin.hModule = LoadLibraryExW(rPlugin.pathShadow.c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
			if (!rPlugin.hModule)
			{
				LogError("plugin {} failed to load (error {})", rPlugin.strName, GetLastError());
				RemoveShadow(rPlugin.pathShadow);
				return nullptr;
			}
			const auto pfnLoad = reinterpret_cast<Plugin::FnPluginLoad>(reinterpret_cast<void*>(GetProcAddress(rPlugin.hModule, Plugin::kLoadExport)));
			rPlugin.pfnUnload = reinterpret_cast<Plugin::FnPluginUnload>(reinterpret_cast<void*>(GetProcAddress(rPlugin.hModule, Plugin::kUnloadExport)));
			if (!pfnLoad || !rPlugin.pfnUnload)
			{
				LogError("{} is not an omegaware plugin (it must export {} and {})", pathFile.filename().string(), Plugin::kLoadExport, Plugin::kUnloadExport);
				FreeLibrary(rPlugin.hModule);
				RemoveShadow(rPlugin.pathShadow);
				return nullptr;
			}

			FillApi(rState, rPlugin);
			bool bLoaded = false;
			{
				auto lckGui = rState.fnLockGui();
				if (!Guarded(rPlugin, "load", [&] { bLoaded = pfnLoad(&rPlugin.stApi); }))
					bLoaded = false;
			}
			if (!bLoaded)
			{
				LogError("plugin {} refused to load (API version {} expected)", rPlugin.strName, Plugin::kApiVersion);
				Unload(rState, std::move(pPlugin), false);
				return nullptr;
			}
			LogInfo("plugin {} loaded with {} feature(s)", rPlugin.strName, rPlugin.aFeatures.size());
			return pPlugin;
		}

		[[nodiscard]] bool IsWritable(const fs::path& pathFile)
		{
			const HANDLE hFile = CreateFileW(pathFile.c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
			if (hFile == INVALID_HANDLE_VALUE)
				return false;
			CloseHandle(hFile);
			return true;
		}

		void RefreshSnapshot(stState& rState)
		{
			std::vector<stPluginInfo> aSnapshot;
			for (const auto& [pathFile, pPlugin] : rState.mapPlugins)
				aSnapshot.push_back({ pPlugin->strName, pPlugin->aFeatures.size(), pPlugin->aHooks.size(), true });
			std::scoped_lock lckSnapshot(rState.mtxSnapshot);
			rState.aSnapshot = std::move(aSnapshot);
		}

		void Scan(stState& rState, bool bImmediate)
		{
			std::error_code errCode;
			std::set<fs::path> setSeen;
			for (const fs::directory_entry& rEntry : fs::directory_iterator(rState.pathDirectory, errCode))
			{
				std::error_code errEntry;
				if (!rEntry.is_regular_file(errEntry))
					continue;
				std::wstring strExtension = rEntry.path().extension().wstring();
				for (wchar_t& rChar : strExtension)
					rChar = static_cast<wchar_t>(std::towlower(rChar));
				if (strExtension != L".dll")
					continue;
				const fs::path& rPath = rEntry.path();
				setSeen.insert(rPath);
				const stObserved stFile{ rEntry.last_write_time(errEntry), rEntry.file_size(errEntry) };

				const auto itLoaded = rState.mapPlugins.find(rPath);
				const bool bUnchanged = itLoaded != rState.mapPlugins.end() && itLoaded->second->tpWrite == stFile.tpWrite && itLoaded->second->uSize == stFile.uSize;
				if (bUnchanged && !bImmediate)
					continue;
				if (const auto itFailed = rState.mapFailed.find(rPath); itFailed != rState.mapFailed.end() && itFailed->second == stFile.tpWrite && !bImmediate)
					continue;

				stObserved& rPending = rState.mapPending[rPath];
				const bool bStable = rPending.tpWrite == stFile.tpWrite && rPending.uSize == stFile.uSize;
				rPending = stFile;
				if ((!bStable && !bImmediate) || !IsWritable(rPath))
					continue;
				rState.mapPending.erase(rPath);

				if (itLoaded != rState.mapPlugins.end())
				{
					LogInfo("plugin {} changed, reloading", itLoaded->second->strName);
					Unload(rState, std::move(itLoaded->second), true);
					rState.mapPlugins.erase(itLoaded);
				}
				if (auto pPlugin = Load(rState, rPath, stFile))
				{
					rState.mapFailed.erase(rPath);
					rState.mapPlugins.emplace(rPath, std::move(pPlugin));
				}
				else
					rState.mapFailed[rPath] = stFile.tpWrite;
			}

			for (auto itPlugin = rState.mapPlugins.begin(); itPlugin != rState.mapPlugins.end();)
			{
				if (setSeen.contains(itPlugin->first))
				{
					++itPlugin;
					continue;
				}
				LogInfo("plugin {} was removed from the plugins folder", itPlugin->second->strName);
				Unload(rState, std::move(itPlugin->second), true);
				itPlugin = rState.mapPlugins.erase(itPlugin);
			}
			RefreshSnapshot(rState);
		}
	}

	void Initialize(const std::filesystem::path& pathDirectory, FnLockGui fnLockGui)
	{
		stState& rState = State();
		if (rState.bInitialized)
			return;
		rState.pathDirectory = pathDirectory;
		rState.fnLockGui = std::move(fnLockGui);
		rState.strDataDirectory = DataDirectory().string();
		rState.strGame = std::string(kGame);
		std::error_code errCode;
		fs::create_directories(pathDirectory, errCode);
		fs::remove_all(pathDirectory / ".loaded", errCode);
		rState.bInitialized = true;
		Scan(rState, true);
		rState.tpNextScan = std::chrono::steady_clock::now() + 1s;
	}

	void Update()
	{
		stState& rState = State();
		if (!rState.bInitialized)
			return;
		const bool bReload = rState.bReloadRequested.exchange(false);
		if (bReload)
		{
			for (auto& [pathFile, pPlugin] : rState.mapPlugins)
			{
				LogInfo("plugin {} reloading on request", pPlugin->strName);
				Unload(rState, std::move(pPlugin), true);
			}
			rState.mapPlugins.clear();
			rState.mapFailed.clear();
			Scan(rState, true);
			return;
		}
		if (std::chrono::steady_clock::now() < rState.tpNextScan)
			return;
		rState.tpNextScan = std::chrono::steady_clock::now() + 1s;
		Scan(rState, false);
	}

	void RequestReload()
	{
		State().bReloadRequested = true;
	}

	void Shutdown()
	{
		stState& rState = State();
		if (!rState.bInitialized)
			return;
		for (auto& [pathFile, pPlugin] : rState.mapPlugins)
			Unload(rState, std::move(pPlugin), true);
		rState.mapPlugins.clear();
		RefreshSnapshot(rState);
		rState.bInitialized = false;
	}

	std::vector<stPluginInfo> List()
	{
		stState& rState = State();
		std::scoped_lock lckSnapshot(rState.mtxSnapshot);
		return rState.aSnapshot;
	}

	std::filesystem::path Directory()
	{
		return State().pathDirectory;
	}
}
