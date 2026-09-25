#include <pch.h>
#include "io/AsyncDownloader.h"

namespace fig::io
{
	AsyncDownloadId AsyncDownloader::Start(const std::string& url, const std::filesystem::path& destination, AsyncDownloadResultDelegate fnDelegate)
	{
		std::lock_guard lock(_mutex);

		FlushFinishedDownloads();

		AsyncDownloadId id = _nextId++;
		auto pDownload = std::make_unique<Download>();
		fig::observer_ptr<Download> download_ptr = pDownload.get();

		pDownload->thread = std::jthread([download_ptr, id, url, destination, fnDelegate](std::stop_token stopToken) {
			DownloadError error = download_ptr->downloader.Download(url, destination, stopToken);
			if (fnDelegate)
				fnDelegate(id, error);
			download_ptr->bComplete = true;
		});

		_downloads[id] = std::move(pDownload);
		return id;
	}

	void AsyncDownloader::Cancel(AsyncDownloadId id)
	{
		std::lock_guard lock(_mutex);
		auto found = _downloads.find(id);
		if (found != _downloads.end())
			found->second->thread.request_stop();
	}

	bool AsyncDownloader::GetProgress(AsyncDownloadId id, std::uint64_t& received, std::uint64_t& total) const
	{
		std::lock_guard lock(_mutex);
		auto itFind = _downloads.find(id);
		if (itFind != _downloads.end())
		{
			received = itFind->second->downloader.GetBytesReceived();
			total = itFind->second->downloader.GetBytesTotal();
			return true;
		}
		return false;
	}

	void AsyncDownloader::FlushFinishedDownloads()
	{
		for (auto it = _downloads.begin(); it != _downloads.end();)
		{
			if (it->second->bComplete)
			{
				it = _downloads.erase(it);
				continue;
			}
			++it;
		}
	}
}