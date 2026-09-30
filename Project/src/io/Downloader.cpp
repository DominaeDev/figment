#include <pch.h>
#include "io/Downloader.h"

#ifdef _WIN32
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

	DownloadError Downloader::Download(const std::string& url, const std::filesystem::path& destination, std::stop_token stopToken)
	{
		_bytesReceived = 0;
		_bytesTotal = 0;

		std::wstring wideUrl = ToWide(url);

		URL_COMPONENTS components = {};
		components.dwStructSize = sizeof(components);
		components.dwSchemeLength = static_cast<DWORD>(-1);
		components.dwHostNameLength = static_cast<DWORD>(-1);
		components.dwUrlPathLength = static_cast<DWORD>(-1);
		components.dwExtraInfoLength = static_cast<DWORD>(-1);

		if (not WinHttpCrackUrl(wideUrl.c_str(), 0, 0, &components))
			return DownloadError::InvalidUrl;

		if (components.lpszHostName == nullptr or components.lpszUrlPath == nullptr)
			return DownloadError::InvalidUrl;

		std::wstring host(components.lpszHostName, components.dwHostNameLength);
		std::wstring path(components.lpszUrlPath, components.dwUrlPathLength + components.dwExtraInfoLength);
		bool secure = components.nScheme == INTERNET_SCHEME_HTTPS;

		std::filesystem::path partPath = destination;
		partPath += ".part";

		std::error_code errorCode;
		uint64_t existingSize = 0;
		if (std::filesystem::exists(partPath, errorCode))
			existingSize = std::filesystem::file_size(partPath, errorCode);

		WinHttpHandle session(WinHttpOpen(L"Figment", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0));
		if (not session)
			return DownloadError::ConnectionFailed;

		WinHttpSetTimeouts(session.get(), 10000, 10000, 10000, 10000);

		WinHttpHandle connection(WinHttpConnect(session.get(), host.c_str(), components.nPort, 0));
		if (not connection)
			return DownloadError::ConnectionFailed;

		DWORD flags = secure ? WINHTTP_FLAG_SECURE : 0;
		WinHttpHandle request(WinHttpOpenRequest(connection.get(), L"GET", path.c_str(), nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags));
		if (not request)
			return DownloadError::RequestFailed;

		if (existingSize > 0)
		{
			std::wstring range = L"Range: bytes=" + std::to_wstring(existingSize) + L"-";
			WinHttpAddRequestHeaders(request.get(), range.c_str(), static_cast<DWORD>(-1), WINHTTP_ADDREQ_FLAG_ADD);
		}

		bool sent = WinHttpSendRequest(request.get(), WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
		if (not sent)
			return DownloadError::RequestFailed;

		bool received = WinHttpReceiveResponse(request.get(), nullptr);
		if (not received)
			return DownloadError::RequestFailed;

		DWORD status = 0;
		DWORD statusSize = sizeof(status);
		WinHttpQueryHeaders(request.get(), WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &status, &statusSize, WINHTTP_NO_HEADER_INDEX);

		bool resuming = status == 206;
		if (status != 200 and not resuming)
			return DownloadError::HttpStatus;

		// Server ignored the range request, start over
		if (not resuming)
			existingSize = 0;

		wchar_t lengthText[32] = {};
		DWORD lengthSize = sizeof(lengthText);
		bool hasLength = WinHttpQueryHeaders(request.get(), WINHTTP_QUERY_CONTENT_LENGTH, WINHTTP_HEADER_NAME_BY_INDEX, lengthText, &lengthSize, WINHTTP_NO_HEADER_INDEX);
		if (hasLength)
			_bytesTotal = existingSize + std::wcstoull(lengthText, nullptr, 10);

		std::ios::openmode mode = std::ios::binary | (resuming ? std::ios::app : std::ios::trunc);
		std::ofstream file(partPath, mode);
		if (not file)
		{
			switch (errno)
			{
			case EACCES:
			case EROFS:
				return DownloadError::WriteAccessError;
			case ENOSPC:
				return DownloadError::DiskFullError;
			default:
				return DownloadError::FileError;
			}
			return DownloadError::FileError;
		}

		_bytesReceived = existingSize;

		std::vector<char> buffer(1 << 16);
		DWORD bytesRead = 0;
		while (true)
		{
			bool read = WinHttpReadData(request.get(), buffer.data(), static_cast<DWORD>(buffer.size()), &bytesRead);
			if (not read or bytesRead == 0)
				break;

			if (stopToken.stop_requested())
				return DownloadError::Cancelled;

			file.write(buffer.data(), bytesRead);
			if (not file)
			{
				switch (errno)
				{
				case EACCES:
				case EROFS:
					return DownloadError::WriteAccessError;
				case ENOSPC:
					return DownloadError::DiskFullError;
				default:
					return DownloadError::FileError;
				}
				return DownloadError::FileError;
			}

			_bytesReceived += bytesRead;
		}

		if (stopToken.stop_requested())
			return DownloadError::Cancelled;

		file.close();
		std::filesystem::rename(partPath, destination, errorCode);
		if (errorCode)
			return DownloadError::FileError;

		return DownloadError::NoError;
	}

	uint64_t Downloader::GetBytesReceived() const
	{
		return _bytesReceived;
	}

	uint64_t Downloader::GetBytesTotal() const
	{
		return _bytesTotal;
	}
}

#else
#error "Not implemented"
#endif