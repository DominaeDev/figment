#include <pch.h>
#include "tts/AudioServerProcess_SDL.h"
#include "io/FileUtility.h"

using namespace fig::gui;

namespace fig::tts
{
	AudioServerProcess_SDL::~AudioServerProcess_SDL()
	{
		Stop();
	}

	bool AudioServerProcess_SDL::Start(const AudioServerConfiguration& config)
	{
		fig::string serverJson = config.ToJson();
		if (auto error = fig::io::WriteTextFile(fig::path { "tts/server.json" }, serverJson); error != fig::io::FileError::NoError)
			return false; // Write error

		const char* arguments[] = { "tts/bin/audiocpp_server.exe", "--config", "tts/server.json", nullptr };

		SDL_PropertiesID properties = SDL_CreateProperties();
		SDL_SetPointerProperty(
			properties,
			SDL_PROP_PROCESS_CREATE_ARGS_POINTER,
			const_cast<void*>(static_cast<const void*>(arguments)));

		SDL_SetNumberProperty(properties, SDL_PROP_PROCESS_CREATE_STDOUT_NUMBER, SDL_PROCESS_STDIO_APP);
		SDL_SetBooleanProperty(properties, SDL_PROP_PROCESS_CREATE_STDERR_TO_STDOUT_BOOLEAN, true);

		_process = SDL_CreateProcessWithProperties(properties);

		SDL_DestroyProperties(properties);

		if (not _process)
			return false;

		SDL_IOStream* output = SDL_GetProcessOutput(_process);
		_logThread = std::jthread(std::bind_front(&AudioServerProcess_SDL::ReadLoop, this, output));
		return true;
	}

	void AudioServerProcess_SDL::Stop()
	{
		if (not _process)
			return;

		SDL_KillProcess(_process, true);

		if (_logThread.joinable())
			_logThread.join();

		SDL_DestroyProcess(_process);
		_process = nullptr;
	}

	bool AudioServerProcess_SDL::IsRunning(int32_t& exitCode)
	{
		if (not _process)
			return false;

		return not SDL_WaitProcess(_process, false, &exitCode);
	}

	void AudioServerProcess_SDL::ReadLoop(SDL_IOStream* output, std::stop_token stopToken)
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
				LogLn("AudioServerProcess_SDL: read failed unexpectedly");

			break;
		}
	}
}