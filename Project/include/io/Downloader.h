#pragma once

#include "Figment.h"
#include <stop_token>

namespace fig::io
{
	enum class DownloadError
	{
		NoError,
		InvalidUrl,
		ConnectionFailed,
		RequestFailed,
		HttpStatus,
		DiskFullError,
		WriteAccessError,
		FileError,
		Cancelled
	};

	class Downloader
	{
	public:
		[[nodiscard]] DownloadError Download(const std::string& url, const std::filesystem::path& destination, std::stop_token stopToken);

		uint64_t GetBytesReceived() const;
		uint64_t GetBytesTotal() const;

	private:
		std::atomic<uint64_t> _bytesReceived = 0;
		std::atomic<uint64_t> _bytesTotal = 0;
	};
}