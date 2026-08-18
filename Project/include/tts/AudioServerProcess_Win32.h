#pragma once

#if defined(_WIN32)

#include "Figment.h"
#include "tts/IAudioServerProcess.h"
#include <windows.h>

namespace fig::tts
{
	class AudioServerProcess_Win32 : public IAudioServerProcess
	{
	public:
		~AudioServerProcess_Win32();

		bool Start(const AudioServerConfiguration& config) override;
		void Stop() override;
		bool IsRunning(int32_t& exitCode) override;

	private:
		HANDLE _hJob { 0 };
		PROCESS_INFORMATION _processInfo {};
		PTP_WAIT _cbWait {};

		static void CALLBACK ProcessEndedCallback(PTP_CALLBACK_INSTANCE Instance, PVOID Context, PTP_WAIT Wait, TP_WAIT_RESULT Result);
		struct CallbackContext
		{
			AudioServerProcess_Win32* pInstance;
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
