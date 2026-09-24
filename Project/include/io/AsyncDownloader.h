#pragma once

#include "Downloader.h"

#include <cstdint>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

namespace fig::io
{
    using AsyncDownloadId = uint32_t;
    using AsyncDownloadResultDelegate = std::function<void(AsyncDownloadId, DownloadError)>;

    class AsyncDownloader
    {
    public:
        ~AsyncDownloader();

        AsyncDownloadId Start(const std::string& url, const std::filesystem::path& destination, AsyncDownloadResultDelegate delegate);
        void Cancel(AsyncDownloadId id);
        bool GetProgress(AsyncDownloadId id, std::uint64_t& received, std::uint64_t& total) const;

    private:
        struct Download
        {
            Downloader downloader;
            std::thread thread;
        };

        mutable std::mutex _mutex;
        std::map<AsyncDownloadId, std::unique_ptr<Download>> _downloads;
        AsyncDownloadId _nextId = {};
    };
}