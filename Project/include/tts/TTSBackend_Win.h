#pragma once

#if defined(_WIN32)

#include "Figment.h"
#include "tts/ITTSBackend.h"
#include "tts/AudioServerProcess_Win.h"
#include "tts/HttpClient_Win.h"

namespace fig::tts
{
	class TTSBackend_Win : public ITTSBackend
	{
	public:
		TTSBackend_Win();
		~TTSBackend_Win();

		bool Initialize() override;
		bool Restart() override;
		void Shutdown() override;

	private:
		bool CheckHealth();
		std::expected<AudioData, TTSError> SendRequest(TTSTask task, fig::uuid modelId, fig::string_view text, fig::string_view instructions, TTSVoiceRef voiceRef) override;

		AudioServerProcess_Win _server {};
		HttpClient_Win _http {};
		bool _bConnected { false };

	};

	using TTSBackend = TTSBackend_Win;
}
#endif
