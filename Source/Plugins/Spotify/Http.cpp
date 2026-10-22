#include "Http.hpp"

#include <Windows.h>
#include <winhttp.h>

#include <array>
#include <format>

#pragma comment(lib, "winhttp.lib")

namespace Spotify::Http
{
	clClient::clClient()
	{
		m_hSession = WinHttpOpen(L"OmegaWare-Spotify/1.0", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
		if (!m_hSession)
			return;
		WinHttpSetTimeouts(m_hSession, 4000, 4000, 4000, 6000);
		DWORD uDecompression = WINHTTP_DECOMPRESSION_FLAG_ALL;
		WinHttpSetOption(m_hSession, WINHTTP_OPTION_DECOMPRESSION, &uDecompression, sizeof(uDecompression));
	}

	clClient::~clClient()
	{
		Cancel();
	}

	void clClient::Cancel()
	{
		std::scoped_lock lckHandles(m_mtxHandles);
		m_bCancelled = true;
		if (m_hRequest)
		{
			WinHttpCloseHandle(m_hRequest);
			m_hRequest = nullptr;
		}
		if (m_hSession)
		{
			WinHttpCloseHandle(m_hSession);
			m_hSession = nullptr;
		}
	}

	stResponse clClient::Get(std::wstring_view strUrl, std::wstring_view strHeaders, std::size_t uMaxBody)
	{
		stResponse stResult;
		const std::wstring strFullUrl(strUrl);
		URL_COMPONENTS stParts{};
		stParts.dwStructSize = sizeof(stParts);
		stParts.dwSchemeLength = static_cast<DWORD>(-1);
		stParts.dwHostNameLength = static_cast<DWORD>(-1);
		stParts.dwUrlPathLength = static_cast<DWORD>(-1);
		stParts.dwExtraInfoLength = static_cast<DWORD>(-1);
		if (!WinHttpCrackUrl(strFullUrl.c_str(), static_cast<DWORD>(strFullUrl.size()), 0, &stParts))
		{
			stResult.uError = GetLastError();
			return stResult;
		}
		const std::wstring strHost(stParts.lpszHostName, stParts.dwHostNameLength);
		std::wstring strPath(stParts.lpszUrlPath, stParts.dwUrlPathLength);
		if (stParts.lpszExtraInfo)
			strPath.append(stParts.lpszExtraInfo, stParts.dwExtraInfoLength);

		HINTERNET hConnect = nullptr;
		HINTERNET hRequest = nullptr;
		{
			std::scoped_lock lckHandles(m_mtxHandles);
			if (m_bCancelled || !m_hSession)
			{
				stResult.uError = ERROR_WINHTTP_OPERATION_CANCELLED;
				return stResult;
			}
			hConnect = WinHttpConnect(m_hSession, strHost.c_str(), stParts.nPort, 0);
			if (hConnect)
				hRequest = WinHttpOpenRequest(hConnect, L"GET", strPath.c_str(), nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, stParts.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0);
			m_hRequest = hRequest;
		}

		const auto fnFinish = [&] {
			std::scoped_lock lckHandles(m_mtxHandles);
			if (m_hRequest == hRequest && hRequest)
			{
				WinHttpCloseHandle(hRequest);
				m_hRequest = nullptr;
			}
			if (hConnect)
				WinHttpCloseHandle(hConnect);
		};

		if (!hRequest)
		{
			stResult.uError = GetLastError();
			fnFinish();
			return stResult;
		}

		const std::wstring strHeaderText(strHeaders);
		const bool bSent = WinHttpSendRequest(hRequest, strHeaderText.empty() ? WINHTTP_NO_ADDITIONAL_HEADERS : strHeaderText.c_str(), strHeaderText.empty() ? 0 : static_cast<DWORD>(-1L), WINHTTP_NO_REQUEST_DATA, 0, 0, 0) && WinHttpReceiveResponse(hRequest, nullptr);
		if (!bSent)
		{
			stResult.uError = GetLastError();
			fnFinish();
			return stResult;
		}

		DWORD uStatus = 0;
		DWORD uLength = sizeof(uStatus);
		WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &uStatus, &uLength, WINHTTP_NO_HEADER_INDEX);
		stResult.uStatus = uStatus;
		DWORD uRetry = 0;
		uLength = sizeof(uRetry);
		if (WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_RETRY_AFTER | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &uRetry, &uLength, WINHTTP_NO_HEADER_INDEX))
			stResult.iRetryAfter = static_cast<int>(uRetry);

		std::array<char, 16384> aChunk{};
		for (;;)
		{
			DWORD uRead = 0;
			if (!WinHttpReadData(hRequest, aChunk.data(), static_cast<DWORD>(aChunk.size()), &uRead))
			{
				stResult.uError = GetLastError();
				break;
			}
			if (!uRead)
				break;
			if (stResult.strBody.size() + uRead > uMaxBody)
			{
				stResult.uError = ERROR_INSUFFICIENT_BUFFER;
				break;
			}
			stResult.strBody.append(aChunk.data(), uRead);
		}
		fnFinish();
		return stResult;
	}

	std::wstring Widen(std::string_view strText)
	{
		if (strText.empty())
			return {};
		const int iLength = MultiByteToWideChar(CP_UTF8, 0, strText.data(), static_cast<int>(strText.size()), nullptr, 0);
		std::wstring strWide(static_cast<std::size_t>(iLength), L'\0');
		MultiByteToWideChar(CP_UTF8, 0, strText.data(), static_cast<int>(strText.size()), strWide.data(), iLength);
		return strWide;
	}

	std::string UrlEncode(std::string_view strText)
	{
		std::string strOut;
		for (const char chCurrent : strText)
		{
			const unsigned char byCurrent = static_cast<unsigned char>(chCurrent);
			if ((byCurrent >= 'A' && byCurrent <= 'Z') || (byCurrent >= 'a' && byCurrent <= 'z') || (byCurrent >= '0' && byCurrent <= '9') || byCurrent == '-' || byCurrent == '_' || byCurrent == '.' || byCurrent == '~')
				strOut += chCurrent;
			else
				strOut += std::format("%{:02X}", byCurrent);
		}
		return strOut;
	}
}
