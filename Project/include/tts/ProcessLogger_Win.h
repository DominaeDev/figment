#pragma once

#if defined(_WIN32)

#include <windows.h>

namespace fig::tts
{
	class ProcessLogger_Win
	{
	public:
		~ProcessLogger_Win();

		std::optional<HANDLE> CreateWriteHandle();
		void Start();
		void Stop();

		ProcessLogger_Win& operator=(const ProcessLogger_Win&) = default;

	private:
		void ReadLoop(std::stop_token stopToken);

		HANDLE _readHandle = nullptr;
		HANDLE _writeHandle = nullptr;
		std::jthread _readThread {};
	};
}
#endif