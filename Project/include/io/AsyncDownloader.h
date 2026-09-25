#pragma once

#include "io/Downloader.h"

namespace fig::io
{
    using AsyncDownloadId = uint32_t;
    using AsyncDownloadResultDelegate = std::function<void(AsyncDownloadId, DownloadError)>;

    class AsyncDownloader
    {
    public:
        AsyncDownloadId Start(const std::string& url, const std::filesystem::path& destination, AsyncDownloadResultDelegate delegate);
        void Cancel(AsyncDownloadId id);
        bool GetProgress(AsyncDownloadId id, std::uint64_t& received, std::uint64_t& total) const;

    private:
        void FlushFinishedDownloads();

        struct Download
        {
            Downloader downloader;
            std::jthread thread;
            std::atomic<bool> bComplete = false;
        };
        std::map<AsyncDownloadId, std::unique_ptr<Download>> _downloads;
        AsyncDownloadId _nextId { 1 };
        mutable std::mutex _mutex;
    };
}