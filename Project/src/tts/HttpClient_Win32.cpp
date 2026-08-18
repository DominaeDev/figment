#include <pch.h>
#include <windows.h>
#include <winhttp.h>

#include "tts/HttpClient_Win32.h"
#include "io/FileUtility.h"

#pragma comment(lib, "winhttp.lib")

namespace fig::tts
{
	HttpClient_Win32::~HttpClient_Win32()
	{
		Disconnect();
	}

	bool HttpClient_Win32::Disconnect()
	{
		if (not IsConnected())
			return false;

		if (_hConnect)
		{
			WinHttpCloseHandle((HINTERNET)_hConnect);
			_hConnect = NULL;
		}

		if (_hSession)
		{
			WinHttpCloseHandle((HINTERNET)_hSession);
			_hSession = NULL;
		}
		return true;
	}

	bool HttpClient_Win32::IsConnected() const 
	{ 
		return (bool)_hSession and (bool)_hConnect;
	}

	bool HttpClient_Win32::Connect(HttpUri hostName, HttpPort port)
	{
		if (IsConnected())
			return false;

		if (_hConnect) // jic
			WinHttpCloseHandle((HINTERNET)_hConnect);
		if (_hSession) // jic
			WinHttpCloseHandle((HINTERNET)_hSession);

		_hSession = WinHttpOpen(L"Figment/1.0", WINHTTP_ACCESS_TYPE_NO_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);

		if (_hSession == NULL)
			return false;

		fig::wstring uri = from_utf8(fig::string { hostName });
		_hConnect = WinHttpConnect((HINTERNET)_hSession, uri.data(), (INTERNET_PORT)port, 0);
		return _hConnect != NULL;
	}

	HttpResponse HttpClient_Win32::Post(fig::string_view endpoint, fig::string_view requestBody)
	{
		fig::wstring wEndpoint { from_utf8(endpoint) };

		HINTERNET request = WinHttpOpenRequest(
			(HINTERNET)_hConnect,
			L"POST",
			wEndpoint.data(),
			nullptr,
			WINHTTP_NO_REFERER,
			WINHTTP_DEFAULT_ACCEPT_TYPES,
			0);

		if (request == NULL)
			return HttpResponse { .responseCode = 404 };

		if (not WinHttpAddRequestHeaders(
			request,
			L"Content-Type: application/json",
			static_cast<DWORD>(-1),
			WINHTTP_ADDREQ_FLAG_ADD))
		{
			WinHttpCloseHandle(request);
			return HttpResponse { .responseCode = 400 };
		}

		if (not WinHttpSendRequest(
			request,
			WINHTTP_NO_ADDITIONAL_HEADERS,
			0,
			const_cast<char*>(requestBody.data()),
			static_cast<DWORD>(requestBody.size()),
			static_cast<DWORD>(requestBody.size()),
			0))
		{
			WinHttpCloseHandle(request);
			return HttpResponse { .responseCode = 400 };
		}

		if (not WinHttpReceiveResponse(request, nullptr))
		{
			WinHttpCloseHandle(request);
			return HttpResponse { .responseCode = 500 };
		}

		DWORD statusCode = 0;
		DWORD statusCodeSize = sizeof(statusCode);

		if (not WinHttpQueryHeaders(
			request,
			WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
			WINHTTP_HEADER_NAME_BY_INDEX,
			&statusCode,
			&statusCodeSize,
			WINHTTP_NO_HEADER_INDEX))
		{
			WinHttpCloseHandle(request);
			return HttpResponse { .responseCode = 422 };
		}

		fig::bytes responseBody;
		DWORD bytesAvailable = 0;
		constexpr size_t MAX_SIZE = 100 * 1024 * 1024; // 100 MB

		while (WinHttpQueryDataAvailable(request, &bytesAvailable) and bytesAvailable > 0)
		{
			if (responseBody.size() + bytesAvailable > MAX_SIZE) // Size cap
			{ 
				WinHttpCloseHandle(request);
				return HttpResponse { .responseCode = 413 };
			}

			size_t offset = responseBody.size();
			responseBody.resize(offset + bytesAvailable);

			DWORD bytesRead = 0;

			if (not WinHttpReadData(request, responseBody.data() + offset, bytesAvailable, &bytesRead))
			{
				WinHttpCloseHandle(request);
				return HttpResponse { .responseCode = 422 };
			}

			if (bytesRead < bytesAvailable)
				responseBody.resize(offset + bytesRead);
		}

		WinHttpCloseHandle(request);
		LogLn(std::format("Audio server returned status code: {}", statusCode));

		if (statusCode != 200)
			LogLn(fig::string { fig::string_view { reinterpret_cast<char*>(responseBody.data()), responseBody.size() } });

		return HttpResponse { 
			.payload = std::move(responseBody),
			.responseCode = static_cast<HttpResponseCode>(statusCode)
		};
	}
}