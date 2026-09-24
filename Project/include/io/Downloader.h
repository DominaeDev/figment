#pragma once

#include <atomic>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <string>

namespace fig::io
{
	enum class DownloadError
	{
		NoError,
		InvalidUrl,
		ConnectionFailed,
		RequestFailed,
		HttpStatus,
		FileError,
		Cancelled,
	};

	class Downloader
	{
	public:
		DownloadError Download(const std::string& url, const std::filesystem::path& destination);

		void Cancel();
		std::uint64_t GetBytesReceived() const;
		std::uint64_t GetBytesTotal() const;

	private:
		std::atomic<bool> _cancelRequested = false;
		std::atomic<std::uint64_t> _bytesReceived = 0;
		std::atomic<std::uint64_t> _bytesTotal = 0;
	};
}