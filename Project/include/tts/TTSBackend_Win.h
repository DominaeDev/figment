#pragma once

#if defined(_WIN32)

#include "Figment.h"
#include <windows.h>
#include "tts/ITTSBackend.h"
#include "tts/ProcessLogger_Win.h"
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
		std::expected<TTSData, TTSError> SendRequest(TTSTask task, fig::string_view text) override;

		static void CALLBACK ProcessEndedCallback(PTP_CALLBACK_INSTANCE Instance, PVOID Context, PTP_WAIT Wait, TP_WAIT_RESULT Result);

		HANDLE _hJob { 0 };
		PROCESS_INFORMATION _processInfo {};
		PTP_WAIT _cbWait {};
		ProcessLogger_Win _logger {};

		struct CallbackContext
		{
			TTSBackend_Win* pInstance;
			HANDLE hProcess;
		} _cbCtx {};

		HttpClient_Win _http {};
		bool _bConnected { false };

	};

	using TTSBackend = TTSBackend_Win;
}
#endif
