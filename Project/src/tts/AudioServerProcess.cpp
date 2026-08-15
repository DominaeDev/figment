#include <pch.h>
#include "tts/AudioServerProcess.h"

using namespace fig::gui;

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

		SDL_SetNumberProperty(properties, SDL_PROP_PROCESS_CREATE_STDOUT_NUMBER, SDL_PROCESS_STDIO_APP);
		SDL_SetBooleanProperty(properties, SDL_PROP_PROCESS_CREATE_STDERR_TO_STDOUT_BOOLEAN, true);

		_process = SDL_CreateProcessWithProperties(properties);

		SDL_DestroyProperties(properties);

		if (not _process)
			return std::unexpected(SDL_GetError());

		SDL_IOStream* output = SDL_GetProcessOutput(_process);
		_logThread = std::jthread(std::bind_front(&AudioServerProcess::ReadLoop, this, output));
		return {};
	}

	void AudioServerProcess::Stop()
	{
		if (not _process)
			return;

		SDL_KillProcess(_process, true);

		if (_logThread.joinable())
			_logThread.join();

		SDL_DestroyProcess(_process);
		_process = nullptr;
	}

	bool AudioServerProcess::IsRunning(int32_t& exitCode)
	{
		if (not _process)
			return false;

		return not SDL_WaitProcess(_process, false, &exitCode);
	}

	void AudioServerProcess::ReadLoop(SDL_IOStream* output, std::stop_token stopToken)
	{
		char buffer[4096];

		while (true)
		{
			size_t bytesRead = SDL_ReadIO(output, buffer, sizeof(buffer));

			if (bytesRead > 0)
			{
				fig::string log { buffer, bytesRead };
				Log(log);

				if (log.contains("ggml_cuda_init"))
					PushEvent(UserEvent::TTSServerLoadingModel);
				else if (log.contains("ggml_backend_cuda_graph_compute"))
					PushEvent(UserEvent::TTSServerGenerating);
				else if (log.contains("audiocpp_server failed"))
					PushEvent(UserEvent::TTSServerError);
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
}