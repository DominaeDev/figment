#include <pch.h>
#include "tts/HttpClient_Win.h"
#include "io/FileUtility.h"
#include "audio/AudioManager.h"

#pragma comment(lib, "winhttp.lib")

namespace fig::tts
{
	HttpClient_Win::~HttpClient_Win()
	{
		Disconnect();
	}

	void HttpClient_Win::Disconnect() noexcept
	{
		if (_connect)
		{
			WinHttpCloseHandle(_connect);
			_connect = NULL;
		}

		if (_session)
		{
			WinHttpCloseHandle(_session);
			_session = NULL;
		}
	}

	bool HttpClient_Win::Connect(std::wstring_view hostName, INTERNET_PORT port)
	{
		if (IsConnected())
			return false;

		_session = WinHttpOpen(
			L"Figment/1.0",
			WINHTTP_ACCESS_TYPE_NO_PROXY,
			WINHTTP_NO_PROXY_NAME,
			WINHTTP_NO_PROXY_BYPASS,
			0);

		if (not _session)
			return false;

		_connect = WinHttpConnect(_session, hostName.data(), port, 0);

		return (bool)_connect;
	}

	std::expected<std::vector<std::byte>, std::error_code> HttpClient_Win::Post(std::wstring_view endpoint, std::string_view jsonBody)
	{
		HINTERNET request = WinHttpOpenRequest(
			_connect,
			L"POST",
			endpoint.data(),
			nullptr,
			WINHTTP_NO_REFERER,
			WINHTTP_DEFAULT_ACCEPT_TYPES,
			0);

		if (not request)
			return std::unexpected(std::error_code(static_cast<int>(GetLastError()), std::system_category()));

		if (not WinHttpAddRequestHeaders(
			request,
			L"Content-Type: application/json",
			static_cast<DWORD>(-1),
			WINHTTP_ADDREQ_FLAG_ADD))
		{
			WinHttpCloseHandle(request);
			return std::unexpected(std::error_code(static_cast<int>(GetLastError()), std::system_category()));
		}

		BOOL sent = WinHttpSendRequest(
			request,
			WINHTTP_NO_ADDITIONAL_HEADERS,
			0,
			const_cast<char*>(jsonBody.data()),
			static_cast<DWORD>(jsonBody.size()),
			static_cast<DWORD>(jsonBody.size()),
			0);

		if (not sent)
		{
			WinHttpCloseHandle(request);
			return std::unexpected(std::error_code(static_cast<int>(GetLastError()), std::system_category()));
		}

		if (not WinHttpReceiveResponse(request, nullptr))
		{
			WinHttpCloseHandle(request);
			return std::unexpected(std::error_code(static_cast<int>(GetLastError()), std::system_category()));
		}

		DWORD statusCode = 0;
		DWORD statusCodeSize = sizeof(statusCode);

		WinHttpQueryHeaders(
			request,
			WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
			WINHTTP_HEADER_NAME_BY_INDEX,
			&statusCode,
			&statusCodeSize,
			WINHTTP_NO_HEADER_INDEX);

		std::vector<std::byte> body;
		DWORD bytesAvailable = 0;

		while (WinHttpQueryDataAvailable(request, &bytesAvailable) and bytesAvailable > 0)
		{
			size_t offset = body.size();
			body.resize(offset + bytesAvailable);

			DWORD bytesRead = 0;

			if (not WinHttpReadData(request, body.data() + offset, bytesAvailable, &bytesRead))
			{
				WinHttpCloseHandle(request);
				return std::unexpected(std::error_code(static_cast<int>(GetLastError()), std::system_category()));
			}

			body.resize(offset + bytesRead);
		}

		WinHttpCloseHandle(request);

		if (statusCode != 200)
			return std::unexpected(std::make_error_code(std::errc::protocol_error));
		LogLn("Received voice clip.");

		Global::GetAudioManager().EnqueueSound(body); //! @temp
		return body;
	}
}