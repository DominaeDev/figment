#pragma once

#if defined(_WIN32)

#include "Figment.h"
#include <windows.h>

namespace fig::tts
{
	class AudioServerProcess_Win
	{
	public:
		~AudioServerProcess_Win();

		std::expected<void, std::string> Start();
		void Stop();
		bool IsRunning(int32_t& exitCode);

	private:
		HANDLE _hJob { 0 };
		PROCESS_INFORMATION _processInfo {};
		PTP_WAIT _cbWait {};

		static void CALLBACK ProcessEndedCallback(PTP_CALLBACK_INSTANCE Instance, PVOID Context, PTP_WAIT Wait, TP_WAIT_RESULT Result);
		struct CallbackContext
		{
			AudioServerProcess_Win* pInstance;
			HANDLE hProcess;
		} _cbCtx {};

		std::optional<HANDLE> CreateWriteHandle();
		void ReadLoop(std::stop_token stopToken);

		HANDLE _readHandle = nullptr;
		HANDLE _writeHandle = nullptr;
		std::jthread _readThread {};
	};
}
#endif
