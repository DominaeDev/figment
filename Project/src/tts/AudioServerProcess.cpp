#include <pch.h>
#include "tts/AudioServerProcess.h"

namespace fig::tts
{
	AudioServerProcess::~AudioServerProcess()
	{
		Stop();
	}

	std::expected<void, std::string> AudioServerProcess::Start(std::span<const char* const> arguments)
	{
		SDL_PropertiesID properties = SDL_CreateProperties();

		SDL_SetPointerProperty(
			properties,
			SDL_PROP_PROCESS_CREATE_ARGS_POINTER,
			const_cast<void*>(static_cast<const void*>(arguments.data())));

#ifdef _DEBUG
		SDL_SetNumberProperty(properties, SDL_PROP_PROCESS_CREATE_STDOUT_NUMBER, SDL_PROCESS_STDIO_APP);
		SDL_SetBooleanProperty(properties, SDL_PROP_PROCESS_CREATE_STDERR_TO_STDOUT_BOOLEAN, true);
#endif

		_process = SDL_CreateProcessWithProperties(properties);

		SDL_DestroyProperties(properties);

		if (not _process)
			return std::unexpected(SDL_GetError());

#ifdef _DEBUG
		SDL_IOStream* output = SDL_GetProcessOutput(_process);
		_logThread = std::jthread(std::bind_front(&AudioServerProcess::ReadLoop, this, output));
#endif
		return {};
	}

	void AudioServerProcess::Stop()
	{
		if (not _process)
			return;

		SDL_KillProcess(_process, true);

#ifdef _DEBUG
		if (_logThread.joinable())
			_logThread.join();
#endif

		SDL_DestroyProcess(_process);
		_process = nullptr;
	}

	bool AudioServerProcess::IsRunning(int32_t& exitCode)
	{
		if (not _process)
			return false;

		return not SDL_WaitProcess(_process, false, &exitCode);
	}

#ifdef _DEBUG
	void AudioServerProcess::ReadLoop(SDL_IOStream* output, std::stop_token stopToken)
	{
		char buffer[4096];

		while (true)
		{
			size_t bytesRead = SDL_ReadIO(output, buffer, sizeof(buffer));

			if (bytesRead > 0)
			{
				Log(fig::string { buffer, bytesRead });
				continue;
			}

			SDL_IOStatus status = SDL_GetIOStatus(output);

			if (status == SDL_IO_STATUS_NOT_READY)
				continue;

			if (status != SDL_IO_STATUS_EOF)
				LogLn("AudioServerProcess: read failed unexpectedly");

			break;
		}
	}
#endif
}