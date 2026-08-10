#pragma once

#if defined(_WIN32)
#include "Figment.h"
#include <windows.h>
#include <winhttp.h>

namespace fig::tts
{
	class HttpClient_Win
	{
	public:
		~HttpClient_Win();

		bool Connect(std::wstring_view hostName, INTERNET_PORT port);
		void Disconnect() noexcept;
		bool IsConnected() const noexcept { return (bool)_session && (bool)_connect; }

		std::expected<std::vector<std::byte>, std::error_code> Post(std::wstring_view endpoint, std::string_view jsonBody);

	private:
		HINTERNET _session = nullptr;
		HINTERNET _connect = nullptr;
	};
}
#endif