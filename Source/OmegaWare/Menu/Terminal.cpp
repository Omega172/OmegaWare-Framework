#include "Menu/Terminal.hpp"
#include "Config/Config.hpp"
#include "Core/Framework.hpp"
#include "Core/Log.hpp"
#if OMEGAWARE_ENGINE_UNITY
#include "Engines/Unity/Il2Cpp.hpp"
#include "Engines/Unity/Mono.hpp"
#endif
#if OMEGAWARE_ENGINE_UNREAL
#include "Engines/Unreal/ProcessEvent.hpp"
#include "Engines/Unreal/Resolver.hpp"
#include "Engines/Unreal/SdkAudit.hpp"
#endif
#include "Features/Feature.hpp"
#include "Plugins/Plugins.hpp"

#include "OmegaGUI/UI/Console.hpp"
#include "OmegaLocalization/OmegaLocalization.hpp"

#include <span>
#include <deque>
#include <mutex>
#include <string>
#include <utility>

namespace OmegaWare::Terminal
{
	namespace
	{
		constexpr std::size_t kPendingLimit = 500;

		std::mutex g_mtxPending;
		std::deque<std::pair<ELogLevel, std::string>> g_aPending;
		OmegaGUI::clConsole g_clConsole;

		[[nodiscard]] OmegaGUI::ELogLevel ToConsoleLevel(ELogLevel eLevel)
		{
			switch (eLevel)
			{
			case ELogLevel::Debug: return OmegaGUI::ELogLevel::Muted;
			case ELogLevel::Warning: return OmegaGUI::ELogLevel::Warning;
			case ELogLevel::Error: return OmegaGUI::ELogLevel::Error;
			default: return OmegaGUI::ELogLevel::Info;
			}
		}

		void OnLog(ELogLevel eLevel, std::string_view strMessage)
		{
			std::scoped_lock lckPending(g_mtxPending);
			g_aPending.emplace_back(eLevel, std::string(strMessage));
			if (g_aPending.size() > kPendingLimit)
				g_aPending.pop_front();
		}

		[[nodiscard]] std::string JoinArguments(std::span<const std::string_view> aArguments, std::size_t uFirst)
		{
			std::string strResult;
			for (std::size_t i = uFirst; i < aArguments.size(); ++i)
				strResult += std::string(aArguments[i]) + (i + 1 < aArguments.size() ? " " : "");
			return strResult;
		}

		void RegisterCommands()
		{
			using OmegaGUI::clConsole;
			using Arguments = std::span<const std::string_view>;

			g_clConsole.RegisterCommand("unload", "unload - remove omegaware from the game", [](clConsole& rConsole, Arguments) {
				rConsole.Log(OmegaGUI::ELogLevel::Warning, "unloading");
				RequestUnload();
			});

			g_clConsole.RegisterCommand("features", "features - list features and their state", [](clConsole& rConsole, Arguments) {
				for (const clFeature* pFeature : Features::All())
					rConsole.Print(pFeature->IsActive() ? OmegaGUI::ELogLevel::Success : OmegaGUI::ELogLevel::Error, "{} - {}", pFeature->GetName(),
						pFeature->IsActive() ? "active" : pFeature->IsFaulted() ? "crashed" : "disabled");
			});

			g_clConsole.RegisterCommand("profile", "profile <list|save|load|default> [name]", [](clConsole& rConsole, Arguments aArguments) {
				const std::string_view strAction = aArguments.size() > 1 ? aArguments[1] : "list";
				const std::string strName = JoinArguments(aArguments, 2);
				if (strAction == "list")
				{
					rConsole.Print(OmegaGUI::ELogLevel::Info, "current: {}, default: {}", Config::GetCurrent(), Config::GetDefault());
					for (const std::string& rProfile : Config::List())
						rConsole.Log(OmegaGUI::ELogLevel::Muted, rProfile);
					return;
				}
				bool bResult = false;
				if (strAction == "save")
					bResult = Config::Save(strName.empty() ? Config::GetCurrent() : strName);
				else if (strAction == "load")
					bResult = Config::Load(strName);
				else if (strAction == "default")
					bResult = Config::SetDefault(strName);
				else
					return rConsole.Log(OmegaGUI::ELogLevel::Warning, "usage: profile <list|save|load|default> [name]");
				if (bResult)
					rConsole.Print(OmegaGUI::ELogLevel::Success, "profile {} done", strAction);
				else
					rConsole.Print(OmegaGUI::ELogLevel::Error, "profile {} failed: {}", strAction, Config::GetLastError());
			});

			g_clConsole.RegisterCommand("lang", "lang [code] - list or switch languages", [](clConsole& rConsole, Arguments aArguments) {
				if (aArguments.size() < 2)
				{
					for (const OmegaLocalization::stLocaleInfo& rLocale : OmegaLocalization::GetLocales())
						rConsole.Print(rLocale.strCode == OmegaLocalization::GetLocale() ? OmegaGUI::ELogLevel::Success : OmegaGUI::ELogLevel::Muted, "{} - {}", rLocale.strCode, rLocale.strName);
					return;
				}
				if (!OmegaLocalization::SetLocale(aArguments[1]))
					return rConsole.Print(OmegaGUI::ELogLevel::Error, "unknown language '{}'", aArguments[1]);
				Config::Add("menu.language", std::string{}) = std::string(OmegaLocalization::GetLocale());
				rConsole.Print(OmegaGUI::ELogLevel::Success, "language set to {}", OmegaLocalization::GetLocale());
			});

			g_clConsole.RegisterCommand("scale", "scale <auto|100|125|150|175|200|250|300>", [](clConsole& rConsole, Arguments aArguments) {
				static constexpr std::string_view aValues[] = { "auto", "100", "125", "150", "175", "200", "250", "300" };
				if (aArguments.size() < 2)
					return rConsole.Log(OmegaGUI::ELogLevel::Warning, "usage: scale <auto|100|125|150|175|200|250|300>");
				for (int i = 0; i < static_cast<int>(std::size(aValues)); ++i)
				{
					if (aArguments[1] != aValues[i] && std::string(aValues[i]) + "%" != aArguments[1])
						continue;
					Config::Add("menu.ui_scale", 0) = i;
					return rConsole.Print(OmegaGUI::ELogLevel::Success, "scale set to {}", aValues[i]);
				}
				rConsole.Print(OmegaGUI::ELogLevel::Error, "unknown scale '{}'", aArguments[1]);
			});

			g_clConsole.RegisterCommand("theme", "theme <console|amber|phosphor>", [](clConsole& rConsole, Arguments aArguments) {
				if (aArguments.size() < 2)
					return rConsole.Log(OmegaGUI::ELogLevel::Warning, "usage: theme <console|amber|phosphor>");
				const int iTheme = aArguments[1] == "amber" ? 1 : aArguments[1] == "phosphor" ? 2 : aArguments[1] == "console" ? 0 : -1;
				if (iTheme < 0)
					return rConsole.Print(OmegaGUI::ELogLevel::Error, "unknown theme '{}'", aArguments[1]);
				Config::Add("menu.theme", 0) = iTheme;
				rConsole.Print(OmegaGUI::ELogLevel::Success, "theme set to {}", aArguments[1]);
			});

#if OMEGAWARE_ENGINE_UNITY
			g_clConsole.RegisterCommand("mono", "mono [check|class <namespace> <name>|method <namespace> <class> <method>] [assembly]", [](clConsole& rConsole, Arguments aArguments) {
				if (!Mono::IsAvailable())
					return rConsole.Log(OmegaGUI::ELogLevel::Warning, "no mono runtime is available");
				const std::string_view strAction = aArguments.size() > 1 ? aArguments[1] : "";
				if (strAction.empty())
					return rConsole.Print(OmegaGUI::ELogLevel::Success, "mono runtime: {}", Mono::GetModuleName());
				if (strAction == "check")
					return rConsole.Print(Mono::LogDiagnostics() ? OmegaGUI::ELogLevel::Success : OmegaGUI::ELogLevel::Error, "mono checks done, see the log");
				const std::string_view strNamespace = aArguments.size() > 2 ? aArguments[2] : "";
				const std::string_view strNamespaceValue = strNamespace == "-" ? std::string_view{} : strNamespace;
				if (strAction == "class" && aArguments.size() > 3)
				{
					const std::string_view strAssembly = aArguments.size() > 4 ? aArguments[4] : Mono::kDefaultAssembly;
					MonoClass* pClass = Mono::GetClass(strNamespaceValue, aArguments[3], strAssembly);
					return rConsole.Print(pClass ? OmegaGUI::ELogLevel::Success : OmegaGUI::ELogLevel::Error, "class {} {}", aArguments[3], pClass ? "found" : "not found");
				}
				if (strAction == "method" && aArguments.size() > 4)
				{
					const std::string_view strAssembly = aArguments.size() > 5 ? aArguments[5] : Mono::kDefaultAssembly;
					void* pCompiled = Mono::GetCompiledMethod(strNamespaceValue, aArguments[3], aArguments[4], -1, strAssembly);
					return rConsole.Print(pCompiled ? OmegaGUI::ELogLevel::Success : OmegaGUI::ELogLevel::Error, "{}.{} {}", aArguments[3], aArguments[4], pCompiled ? std::format("compiled at {}", pCompiled) : std::string("not found"));
				}
				rConsole.Log(OmegaGUI::ELogLevel::Warning, "usage: mono [check|class <namespace|-> <name>|method <namespace|-> <class> <method>] [assembly]");
			});

#endif

#if OMEGAWARE_ENGINE_UNREAL
			g_clConsole.RegisterCommand("unreal", "unreal [check|hooks|globals|scan|fields|audit]", [](clConsole& rConsole, Arguments aArguments) {
				const std::string_view strAction = aArguments.size() > 1 ? aArguments[1] : "";
				if (strAction == "check")
					return rConsole.Print(Unreal::LogDiagnostics() ? OmegaGUI::ELogLevel::Success : OmegaGUI::ELogLevel::Error, "unreal checks done, see the log");
				if (strAction == "globals")
				{
					Unreal::Resolver::LogReport();
					return rConsole.Print(Unreal::IsResolved() ? OmegaGUI::ELogLevel::Success : OmegaGUI::ELogLevel::Error, "unreal globals {}, see the log", Unreal::IsResolved() ? "resolved" : "not resolved");
				}
				if (strAction == "scan")
					return rConsole.Print(Unreal::Resolver::LogScanTest() ? OmegaGUI::ELogLevel::Success : OmegaGUI::ELogLevel::Error, "unreal scan test done, see the log");
				if (strAction == "audit")
				{
					const Unreal::SdkAudit::stSummary stAudit = Unreal::SdkAudit::Run();
					if (!stAudit.bRan)
						return rConsole.Log(OmegaGUI::ELogLevel::Error, "unreal audit did not run, see the log");
					return rConsole.Print(stAudit.uMoved ? OmegaGUI::ELogLevel::Warning : OmegaGUI::ELogLevel::Success, "{} members: {} match, {} moved, {} not reflected, {} not loaded ({} ms), report in {}", stAudit.uMembers, stAudit.uMatched, stAudit.uMoved,
						stAudit.uNotReflected, stAudit.uOwnerMissing, stAudit.llMilliseconds, stAudit.pathReport.string());
				}
				if (strAction == "fields")
				{
					const auto aFields = Unreal::ResolvedFields();
					if (aFields.empty())
						return rConsole.Log(OmegaGUI::ELogLevel::Info, "no fields resolved yet");
					for (const Unreal::stFieldInfo& rField : aFields)
					{
						const bool bMoved = rField.eStatus == Unreal::EFieldStatus::Moved;
						const bool bMatches = rField.eStatus == Unreal::EFieldStatus::Matches;
						rConsole.Print(bMatches ? OmegaGUI::ELogLevel::Success : bMoved ? OmegaGUI::ELogLevel::Warning : OmegaGUI::ELogLevel::Error, "{}::{} SDK 0x{:X} game 0x{:X} ({})", rField.strClass, rField.strMember, rField.uSdkOffset, rField.uGameOffset,
							bMatches ? "matches" : bMoved ? "moved" : rField.eStatus == Unreal::EFieldStatus::NotReflected ? "not reflected" : "class not found");
					}
					return;
				}
				if (strAction == "hooks")
				{
					const auto aHooks = Unreal::ProcessEventHooks();
					if (aHooks.empty())
						return rConsole.Log(OmegaGUI::ELogLevel::Info, "no process event hooks");
					for (const Unreal::stProcessEventHookInfo& rHook : aHooks)
						rConsole.Print(rHook.bFaulted ? OmegaGUI::ELogLevel::Error : rHook.bResolved ? OmegaGUI::ELogLevel::Success : OmegaGUI::ELogLevel::Warning, "#{} {} ({}, {} calls)", rHook.uHandle, rHook.strFunction,
							rHook.bFaulted ? "faulted" : rHook.bResolved ? "active" : "waiting for the function to load", rHook.uCalls);
					return;
				}
				rConsole.Log(OmegaGUI::ELogLevel::Warning, "usage: unreal [check|hooks|globals|scan|fields|audit]");
			});
#endif

#if OMEGAWARE_ENGINE_UNITY
			g_clConsole.RegisterCommand("il2cpp", "il2cpp [check|class <namespace> <name>|method <namespace> <class> <method>] [assembly]", [](clConsole& rConsole, Arguments aArguments) {
				if (!Il2Cpp::IsAvailable())
					return rConsole.Log(OmegaGUI::ELogLevel::Warning, "no il2cpp runtime is available");
				const std::string_view strAction = aArguments.size() > 1 ? aArguments[1] : "";
				if (strAction.empty())
					return rConsole.Print(OmegaGUI::ELogLevel::Success, "il2cpp runtime: {}", Il2Cpp::GetModuleName());
				if (strAction == "check")
					return rConsole.Print(Il2Cpp::LogDiagnostics() ? OmegaGUI::ELogLevel::Success : OmegaGUI::ELogLevel::Error, "il2cpp checks done, see the log");
				const std::string_view strNamespace = aArguments.size() > 2 ? aArguments[2] : "";
				const std::string_view strNamespaceValue = strNamespace == "-" ? std::string_view{} : strNamespace;
				if (strAction == "class" && aArguments.size() > 3)
				{
					const std::string_view strAssembly = aArguments.size() > 4 ? aArguments[4] : Il2Cpp::kDefaultAssembly;
					Il2CppClass* pClass = Il2Cpp::GetClass(strNamespaceValue, aArguments[3], strAssembly);
					return rConsole.Print(pClass ? OmegaGUI::ELogLevel::Success : OmegaGUI::ELogLevel::Error, "class {} {}", aArguments[3], pClass ? "found" : "not found");
				}
				if (strAction == "method" && aArguments.size() > 4)
				{
					const std::string_view strAssembly = aArguments.size() > 5 ? aArguments[5] : Il2Cpp::kDefaultAssembly;
					void* pCode = Il2Cpp::GetCompiledMethod(strNamespaceValue, aArguments[3], aArguments[4], -1, strAssembly);
					return rConsole.Print(pCode ? OmegaGUI::ELogLevel::Success : OmegaGUI::ELogLevel::Error, "{}.{} {}", aArguments[3], aArguments[4], pCode ? std::format("code at {}", pCode) : std::string("not found"));
				}
				rConsole.Log(OmegaGUI::ELogLevel::Warning, "usage: il2cpp [check|class <namespace|-> <name>|method <namespace|-> <class> <method>] [assembly]");
			});

#endif

			g_clConsole.RegisterCommand("plugins", "plugins [list|reload|folder]", [](clConsole& rConsole, Arguments aArguments) {
				const std::string_view strAction = aArguments.size() > 1 ? aArguments[1] : "list";
				if (strAction == "reload")
				{
					Plugins::RequestReload();
					return rConsole.Log(OmegaGUI::ELogLevel::Success, "reloading every plugin");
				}
				if (strAction == "folder")
					return rConsole.Print(OmegaGUI::ELogLevel::Info, "{}", Plugins::Directory().string());
				const auto aPlugins = Plugins::List();
				if (aPlugins.empty())
					return rConsole.Print(OmegaGUI::ELogLevel::Muted, "no plugins loaded, drop dlls into {}", Plugins::Directory().string());
				for (const Plugins::stPluginInfo& rPlugin : aPlugins)
					rConsole.Print(OmegaGUI::ELogLevel::Success, "{} - {} feature(s), {} hook(s)", rPlugin.strName, rPlugin.uFeatures, rPlugin.uHooks);
			});

			g_clConsole.RegisterCommand("toast", "toast <text> - show a notification", [](clConsole&, Arguments aArguments) {
				const std::string strText = JoinArguments(aArguments, 1);
				OmegaGUI::PushToast(OmegaGUI::EToastKind::Info, strText.empty() ? "hello from the console" : strText);
			});
		}
	}

	void Initialize()
	{
		RegisterCommands();
		g_clConsole.SetCompletion("profile", [](std::span<const std::string_view> aArguments) {
			if (aArguments.size() == 1)
				return std::vector<std::string>{ "list", "save", "load", "default" };
			if (aArguments.size() >= 2 && (aArguments[1] == "save" || aArguments[1] == "load" || aArguments[1] == "default"))
				return Config::List();
			return std::vector<std::string>{};
		});
		g_clConsole.SetCompletion("lang", [](std::span<const std::string_view> aArguments) {
			std::vector<std::string> aCodes;
			if (aArguments.size() == 1)
				for (const OmegaLocalization::stLocaleInfo& rLocale : OmegaLocalization::GetLocales())
					aCodes.emplace_back(rLocale.strCode);
			return aCodes;
		});
		SetLogListener(&OnLog);
	}

	void Shutdown()
	{
		SetLogListener(nullptr);
	}

	void Draw(bool* pOpen)
	{
		{
			std::scoped_lock lckPending(g_mtxPending);
			for (const auto& [eLevel, strMessage] : g_aPending)
				g_clConsole.Log(ToConsoleLevel(eLevel), strMessage);
			g_aPending.clear();
		}
		if (!*pOpen)
			return;
		g_clConsole.pAlwaysVisible = &Config::Add("menu.console_pinned", false);
		const OmegaGUI::vec2 vecDisplay = OmegaGUI::GetDisplaySize();
		OmegaGUI::SetNextWindowPos({ vecDisplay.x - 660.f, vecDisplay.y - 420.f });
		g_clConsole.Draw("console", pOpen);
	}
}
