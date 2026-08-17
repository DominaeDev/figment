#pragma once

#include "Figment.h"
#include "tts/TTSTypes.h"
#include "tts/VoiceModelSettings.h"

namespace fig::tts
{
	struct TTSVoiceRef 
	{
		fig::observer_ptr<const AudioData> pData;
		fig::string referenceText;
	};

	class ITTSBackend
	{
	public:
		virtual ~ITTSBackend();

		virtual bool Initialize() = 0;
		virtual void Shutdown() = 0;
		virtual bool Restart() = 0;
	
		TTSStatus GetStatus() const noexcept { return _status; };

		void UnloadAllModels();
		void UnloadSpeechModels();
		void UnloadDesignModels();

		std::expected<std::vector<TTSResult>, TTSError> Speak(fig::uuid characterId, fig::string_view text, bool split = true);
		std::expected<TTSResult, TTSError> Design(fig::string_view text, fig::string_view instruct);

	protected:
		ITTSBackend();
		virtual TTSPayload SendRequest(TTSTask task, fig::uuid modelId, fig::string_view text, fig::string_view instructions, TTSVoiceRef voiceRef) = 0;
		void LoadModelConfigurations();

	protected:
		TTSStatus _status { TTSStatus::Uninitialized };
		fig::tts::VoiceModelSettings _models;

		struct PendingRequest {
			uint64_t id {};
			TTSTask task {};
			fig::uuid modelId {};
			fig::string text {};
			fig::string instructions {};
			TTSVoiceRef voiceReference {};
			std::unique_ptr<TTSPromise> promise;
		};
		bool IsAsyncRequestAlive(const PendingRequest& request) const;
		std::optional<TTSResult> EnqueueTask(TTSTask task, fig::uuid modelId, fig::uuid characterId, fig::string_view text, fig::string_view instructions = {});

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
		TTSPayload SendRequest(TTSTask task, fig::uuid modelId, fig::string_view text, fig::string_view instructions, TTSVoiceRef voiceRef) override { return std::unexpected(TTSError::Unavailable); }
	};
}

