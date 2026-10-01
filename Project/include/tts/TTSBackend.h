#pragma once

#include "Figment.h"
#include "tts/TTSTypes.h"
#include "tts/TTSModelSettings.h"
#include "tts/TTSBackendSettings.h"

namespace fig::tts
{
	struct TTSVoiceRef 
	{
		fig::observer_ptr<const AudioData> pData {};
		fig::string referenceText;
	};

	struct TTSTaskArguments
	{
		fig::uuid modelId;
		fig::string text;
		fig::string instructions;
		TTSVoiceRef voiceReference;
		uint32_t seed {};
	};

	class TTSBackend
	{
	public:
		TTSBackend();
		virtual ~TTSBackend();

		bool Initialize();
		void Shutdown();
		bool Restart();
	
		TTSStatus GetStatus() const noexcept { return _status; };

		void UnloadAllModels();
		void UnloadSpeechModels();
		void UnloadDesignModels();

		std::expected<std::vector<TTSResult>, TTSError> Speak(fig::uuid characterId, fig::string_view text, bool split = true);
		std::expected<TTSResult, TTSError> Design(fig::string_view text, fig::string_view instruct, uint32_t seed = 0);

		std::vector<fig::tts::VoiceModel> GetVoiceModels() const;
		std::vector<fig::tts::TTSBackendInfo> GetBackendSettings() const;

	protected:
		TTSPayload SendRequest(TTSTask task, TTSTaskArguments args);
		void LoadTTSSettings();
		bool CheckHealth();
		fig::optional_cref<TTSBackendInfo> GetActiveBackend() const;

	protected:
		TTSStatus _status { TTSStatus::Uninitialized };
		fig::tts::TTSModelSettings _ttsModels;
		fig::tts::TTSBackendSettings _ttsBackends;
		std::unique_ptr<class IAudioServerProcess> _pServer {};
		std::unique_ptr<class IHttpClient> _pHttp {};
		bool _bConnected { false };

		struct PendingRequest {
			uint64_t id {};
			TTSTask task {};
			TTSTaskArguments args {};
			std::unique_ptr<TTSPromise> promise;
		};
		bool IsAsyncRequestAlive(const PendingRequest& request) const;
		std::optional<TTSResult> EnqueueTask(TTSTask task, TTSTaskArguments args);

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
}

