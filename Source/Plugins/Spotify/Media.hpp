#pragma once

#include <memory>

namespace Spotify::Media
{
	enum class ECommand
	{
		PlayPause,
		Next,
		Previous,
	};

	enum class EResult
	{
		Sent,
		NoSession,
		Failed,
	};

	class clController
	{
	public:
		clController();
		~clController();

		clController(const clController&) = delete;
		clController& operator=(const clController&) = delete;

		[[nodiscard]] EResult Send(ECommand eCommand);

	private:
		struct stImpl;
		std::unique_ptr<stImpl> m_pImpl;
	};
}
