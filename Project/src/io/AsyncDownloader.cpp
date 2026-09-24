#include <pch.h>
#include "io/AsyncDownloader.h"

namespace fig::io
{
	AsyncDownloader::~AsyncDownloader()
	{
		for (auto& kvp : _downloads)
			kvp.second->downloader.Cancel();

		for (auto& kvp : _downloads)
			kvp.second->thread.join();
	}

	AsyncDownloadId AsyncDownloader::Start(const std::string& url, const std::filesystem::path& destination, AsyncDownloadResultDelegate callback)
	{
		std::lock_guard lock(_mutex);

		AsyncDownloadId id = _nextId++;
		auto download = std::make_unique<Download>();
		fig::observer_ptr<Download> pDownload = download.get();

		download->thread = std::thread([pDownload, id, url, destination, callback] {
			DownloadError error = pDownload->downloader.Download(url, destination);
			callback(id, error);
		});

		_downloads[id] = std::move(download);
		return id;
	}

	void AsyncDownloader::Cancel(AsyncDownloadId id)
	{
		std::lock_guard lock(_mutex);
		auto found = _downloads.find(id);
		if (found != _downloads.end())
			found->second->downloader.Cancel();
	}

	bool AsyncDownloader::GetProgress(AsyncDownloadId id, std::uint64_t& received, std::uint64_t& total) const
	{
		std::lock_guard lock(_mutex);
		auto found = _downloads.find(id);
		if (found == _downloads.end())
			return false;

		received = found->second->downloader.GetBytesReceived();
		total = found->second->downloader.GetBytesTotal();
		return true;
	}
}