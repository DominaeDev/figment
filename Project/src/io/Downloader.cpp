#include <pch.h>
#include "io/Downloader.h"

#ifdef _WIN32
#include <fstream>
#include <memory>
#include <vector>
#include <windows.h>
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")

namespace fig::io
{
	struct WinHttpHandleCloser
	{
		void operator()(void* handle) const
		{
			WinHttpCloseHandle(handle);
		}
	};

	using WinHttpHandle = std::unique_ptr<void, WinHttpHandleCloser>;

	static std::wstring ToWide(const std::string& text)
	{
		int length = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
		std::wstring result(length, L'\0');
		MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, result.data(), length);
		result.resize(length - 1);
		return result;
	}

	DownloadError Downloader::Download(const std::string& url, const std::filesystem::path& destination)
	{
		_cancelRequested = false;
		_bytesReceived = 0;
		_bytesTotal = 0;

		std::wstring wideUrl = ToWide(url);

		URL_COMPONENTS components = {};
		components.dwStructSize = sizeof(components);
		components.dwSchemeLength = static_cast<DWORD>(-1);
		components.dwHostNameLength = static_cast<DWORD>(-1);
		components.dwUrlPathLength = static_cast<DWORD>(-1);
		components.dwExtraInfoLength = static_cast<DWORD>(-1);

		if (!WinHttpCrackUrl(wideUrl.c_str(), 0, 0, &components))
			return DownloadError::InvalidUrl;
		if (components.lpszHostName == NULL or components.lpszUrlPath == NULL)
			return DownloadError::InvalidUrl;

		std::wstring host(components.lpszHostName, components.dwHostNameLength);
		std::wstring path(components.lpszUrlPath, components.dwUrlPathLength + components.dwExtraInfoLength);
		bool secure = components.nScheme == INTERNET_SCHEME_HTTPS;

		std::filesystem::path partPath = destination;
		partPath += ".part";

		std::error_code errorCode;
		std::uint64_t existingSize = 0;
		if (std::filesystem::exists(partPath, errorCode))
		{
			existingSize = std::filesystem::file_size(partPath, errorCode);
		}

		WinHttpHandle session(WinHttpOpen(L"Figment", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0));
		if (!session)
			return DownloadError::ConnectionFailed;

		WinHttpHandle connection(WinHttpConnect(session.get(), host.c_str(), components.nPort, 0));
		if (!connection)
			return DownloadError::ConnectionFailed;

		WinHttpHandle request(WinHttpOpenRequest(connection.get(), L"GET", path.c_str(), nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, secure ? WINHTTP_FLAG_SECURE : 0));
		if (!request)
			return DownloadError::RequestFailed;

		if (existingSize > 0)
		{
			std::wstring range = L"Range: bytes=" + std::to_wstring(existingSize) + L"-";
			WinHttpAddRequestHeaders(request.get(), range.c_str(), static_cast<DWORD>(-1), WINHTTP_ADDREQ_FLAG_ADD);
		}

		if (!WinHttpSendRequest(request.get(), WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) || !WinHttpReceiveResponse(request.get(), nullptr))
		{
			return DownloadError::RequestFailed;
		}

		DWORD status = 0;
		DWORD statusSize = sizeof(status);
		WinHttpQueryHeaders(request.get(), WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &status, &statusSize, WINHTTP_NO_HEADER_INDEX);

		bool bResuming = status == 206;
		if (status != 200 && !bResuming)
			return DownloadError::HttpStatus;

		if (!bResuming) // Server ignored the range request, start over
			existingSize = 0;

		DWORD contentLength = 0;
		DWORD lengthSize = sizeof(contentLength);
		if (WinHttpQueryHeaders(request.get(), WINHTTP_QUERY_CONTENT_LENGTH | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &contentLength, &lengthSize, WINHTTP_NO_HEADER_INDEX))
			_bytesTotal = existingSize + contentLength;

		std::ofstream file(partPath, std::ios::binary | (bResuming ? std::ios::app : std::ios::trunc));
		if (!file)
			return DownloadError::FileError;

		_bytesReceived = existingSize;

		std::vector<char> buffer(1 << 16);
		DWORD bytesRead = 0;
		while (WinHttpReadData(request.get(), buffer.data(), static_cast<DWORD>(buffer.size()), &bytesRead) && bytesRead > 0)
		{
			if (_cancelRequested)
				return DownloadError::Cancelled;

			file.write(buffer.data(), bytesRead);
			if (!file)
				return DownloadError::FileError;

			_bytesReceived += bytesRead;
		}

		file.close();
		std::filesystem::rename(partPath, destination, errorCode);
		if (errorCode)
			return DownloadError::FileError;

		return DownloadError::NoError;
	}

	void Downloader::Cancel()
	{
		_cancelRequested = true;
	}

	std::uint64_t Downloader::GetBytesReceived() const
	{
		return _bytesReceived;
	}

	std::uint64_t Downloader::GetBytesTotal() const
	{
		return _bytesTotal;
	}
}
#else
#error "Not implemented"
#endif