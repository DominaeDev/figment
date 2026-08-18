#pragma once

#include "Figment.h"

namespace fig::tts
{
	enum class HttpError
	{
		NoError,
		ConnectionFailed,
	};

	using HttpUri = fig::string_view;
	using HttpPort = int32_t;
	using HttpResponseCode = int32_t;

	struct HttpResponse
	{
		fig::bytes payload;
		HttpResponseCode responseCode;

		explicit operator bool() const noexcept { return not payload.empty(); }
	};

	class IHttpClient
	{
	public:
		virtual ~IHttpClient() {};
		virtual bool Connect(HttpUri hostName, HttpPort port) = 0;
		virtual bool Disconnect() = 0;
		virtual bool IsConnected() const = 0;
		virtual HttpResponse Post(fig::string_view endpoint, fig::string_view requestBody) = 0;
	};

	class HttpClient_Dummy : public IHttpClient
	{
	public:
		bool Connect(fig::string_view hostName, HttpPort port) override { return false; }
		bool Disconnect() override { return false; }
		bool IsConnected() const override { return false; }
		HttpResponse Post(fig::string_view endpoint, fig::string_view requestBody) override { return { .responseCode = 500 }; }
	};
}