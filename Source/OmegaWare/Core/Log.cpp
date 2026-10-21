#include "Core/Log.hpp"
#include "Core/Console.hpp"

#include <Windows.h>

#include <atomic>
#include <chrono>
#include <format>
#include <fstream>
#include <mutex>

namespace OmegaWare
{
	namespace
	{
		std::mutex g_mtxLog;
		std::ofstream g_fileLog;
		std::ofstream g_fileErrors;
		std::filesystem::path g_pathErrors;
		std::atomic<LogListener> g_pfnListener = nullptr;

		void WriteHookLog(ELogLevel eLevel, std::string_view strMessage)
		{
			WriteLog(eLevel, strMessage);
		}

		[[nodiscard]] std::string LaunchTimestamp()
		{
			return std::format("{:%Y-%m-%d_%H-%M-%S}", std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now()));
		}

		void OpenExclusive(std::ofstream& rFile, const std::filesystem::path& pathFile)
		{
			rFile.open(pathFile, std::ios::out | std::ios::app, _SH_DENYWR);
			if (rFile.is_open())
				return;
			std::filesystem::path pathOwn = pathFile;
			pathOwn.replace_filename(std::format("{}-{}{}", pathFile.stem().string(), GetCurrentProcessId(), pathFile.extension().string()));
			rFile.open(pathOwn, std::ios::out | std::ios::app, _SH_DENYWR);
		}

		[[nodiscard]] std::string FunctionName(std::string_view strSignature)
		{
			int iDepth = 0;
			std::size_t uParameters = std::string_view::npos;
			for (std::size_t i = 0; i < strSignature.size(); ++i)
			{
				if (strSignature[i] == '<')
					++iDepth;
				else if (strSignature[i] == '>' && iDepth > 0)
					--iDepth;
				else if (strSignature[i] == '(' && iDepth == 0 && !strSignature.substr(0, i).ends_with("operator "))
				{
					uParameters = i;
					break;
				}
			}
			std::string_view strName = strSignature.substr(0, uParameters);
			iDepth = 0;
			for (std::size_t i = strName.size(); i-- > 0;)
			{
				if (strName[i] == '>')
					++iDepth;
				else if (strName[i] == '<' && iDepth > 0)
					--iDepth;
				else if (strName[i] == ' ' && iDepth == 0 && !strName.substr(0, i).ends_with("operator"))
				{
					strName = strName.substr(i + 1);
					break;
				}
			}
			return std::string(strName);
		}
	}

	void SetLogListener(LogListener pfnListener)
	{
		g_pfnListener = pfnListener;
	}

	void OpenLog(const std::filesystem::path& pathDirectory)
	{
		std::scoped_lock lckLog(g_mtxLog);
		std::error_code errCode;
		std::filesystem::create_directories(pathDirectory, errCode);
		const std::string strTimestamp = LaunchTimestamp();
		OpenExclusive(g_fileLog, pathDirectory / (strTimestamp + ".log"));
		g_pathErrors = pathDirectory / (strTimestamp + "_ERRORS.log");
		OmegaRenderHook::SetLogSink(&WriteHookLog);
	}

	void CloseLog()
	{
		OmegaRenderHook::SetLogSink(nullptr);
		std::scoped_lock lckLog(g_mtxLog);
		g_fileLog.close();
		g_fileErrors.close();
	}

	std::string FormatLocation(const std::source_location& rLocation)
	{
		return std::format("[{} ({},{})] {} |", std::filesystem::path(rLocation.file_name()).filename().string(), rLocation.line(), rLocation.column(), FunctionName(rLocation.function_name()));
	}

	void WriteLog(ELogLevel eLevel, std::string_view strMessage, const std::source_location* pLocation)
	{
		const auto tpNow = std::chrono::floor<std::chrono::milliseconds>(std::chrono::system_clock::now());
		const bool bLocated = pLocation && (eLevel == ELogLevel::Warning || eLevel == ELogLevel::Error);
		const std::string strMessageLine = bLocated ? std::format("{} {}", FormatLocation(*pLocation), strMessage) : std::string(strMessage);
		const std::string strLine = std::format("[{:%T}] [{}] [tid {}] {}", tpNow, OmegaRenderHook::ToString(eLevel), GetCurrentThreadId(), strMessageLine);

		{
			std::scoped_lock lckLog(g_mtxLog);
			if (g_fileLog)
			{
				g_fileLog << strLine << '\n';
				g_fileLog.flush();
			}
			if (eLevel == ELogLevel::Error && !g_pathErrors.empty())
			{
				if (!g_fileErrors.is_open())
					OpenExclusive(g_fileErrors, g_pathErrors);
				if (g_fileErrors)
				{
					g_fileErrors << strLine << '\n';
					g_fileErrors.flush();
				}
			}
		}
		OutputDebugStringA((strLine + "\n").c_str());
		Console::Write(eLevel, strLine);
		if (const LogListener pfnListener = g_pfnListener)
			pfnListener(eLevel, strMessageLine);
	}
}
