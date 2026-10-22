#pragma once

#include <mutex>
#include <string>
#include <string_view>

namespace Spotify::Http
{
	struct stResponse
	{
		unsigned long uStatus = 0;
		unsigned long uError = 0;
		int iRetryAfter = 0;
		std::string strBody;
	};

	class clClient
	{
	public:
		clClient();
		~clClient();

		clClient(const clClient&) = delete;
		clClient& operator=(const clClient&) = delete;

		[[nodiscard]] stResponse Get(std::wstring_view strUrl, std::wstring_view strHeaders = {}, std::size_t uMaxBody = 4 * 1024 * 1024);
		void Cancel();

	private:
		std::mutex m_mtxHandles;
		void* m_hSession = nullptr;
		void* m_hRequest = nullptr;
		bool m_bCancelled = false;
	};

	[[nodiscard]] std::wstring Widen(std::string_view strText);
	[[nodiscard]] std::string UrlEncode(std::string_view strText);
}
