#pragma once

#include "Figment.h"
#include <future>

namespace fig::tts
{
	enum class TTSError
	{
		NoError = 0,
		Failed,
		Canceled,
		Busy,			// Server responded with 503 busy
		Unavailable,	// Server didn't respond
	};

	using TTSData = fig::bytes;
	using TTSPayload = std::expected<TTSData, TTSError>;
	using TTSPromise = std::promise<TTSPayload>;
	using TTSFuture = std::future<TTSPayload>;

	enum class TTSTask
	{
		Speak = 0,
		Design,
	};

	struct TTSResult
	{
		uint64_t id;
		TTSTask task;
		TTSFuture future;
	};

	enum TTSStatus
	{
		Uninitialized,
		ServerStarted,
		ServerRunning,
		ServerBusy,
	};

	class ITTSBackend
	{
	public:
		virtual ~ITTSBackend();

		virtual bool Initialize() = 0;
		virtual void Shutdown() = 0;
		virtual bool Restart() = 0;
	
		TTSStatus GetStatus() const noexcept { return _status; };

		bool Speak(fig::string_view text, bool split = true);
		TTSResult EnqueueTask(TTSTask task, const fig::string& text);

	protected:
		ITTSBackend();
		virtual TTSPayload SendRequest(TTSTask task, fig::string_view text) = 0;

	protected:
		TTSStatus _status { TTSStatus::Uninitialized };

		struct PendingRequest {
			uint64_t id {};
			TTSTask task {};
			fig::string text {};
			std::unique_ptr<TTSPromise> promise;
		};
		bool IsAsyncRequestAlive(const PendingRequest& request) const;

	private:
		mutable std::mutex _pending_mutex;
		std::queue<PendingRequest> _pending;
		std::condition_variable _pending_cv;
		mutable std::mutex _active_mutex;
		std::map<uint64_t, TTSPromise*> _active_promises;
		std::atomic<uint64_t> _next_id { 0 };

		void __Worker(std::stop_token stop);
		std::jthread _worker;
	};

	class TTSBackend_Dummy : public ITTSBackend
	{
	public:
		bool Initialize() override { return false; }
		bool Restart() override { return false; }
		void Shutdown() override {}
	protected:
		TTSPayload SendRequest(TTSTask task, fig::string_view text) override { return std::unexpected(TTSError::Failed); }
	};
}

