#pragma once

#include "tts/IHttpClient.h"

#if defined(_WIN32)

namespace fig::tts
{
	class HttpClient_Win32 : public IHttpClient
	{
	public:
		~HttpClient_Win32();

		bool Connect(HttpUri hostName, HttpPort port) override;
		bool Disconnect() override;
		bool IsConnected() const override;

		HttpResponse Post(fig::string_view endpoint, fig::string_view requestBody) override;

	private:
		void* _hSession = nullptr;
		void* _hConnect = nullptr;
	};
}
#endif