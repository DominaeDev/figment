#include <pch.h>
#include "tts/TTSBackend_Win.h"

using namespace fig::gui;

namespace fig::tts
{
	TTSBackend_Win::TTSBackend_Win() : ITTSBackend()
	{
	}

	TTSBackend_Win::~TTSBackend_Win()
	{
		Shutdown();
	}

	bool TTSBackend_Win::Initialize()
	{
		if (_status != TTSStatus::Uninitialized)
		{
			if (CheckHealth())
				return true; // Already initialized
			Shutdown(); // Restart
		}

		if (auto started = _server.Start())
		{
			_status = TTSStatus::ServerStarted;
			PushEvent(UserEvent::TTSServerStarted);
			return true;
		}
		else
		{
			PushEvent(UserEvent::TTSServerShutdown);
			return false;
		}
	}

	void TTSBackend_Win::Shutdown()
	{
		// Shut down server
		int32_t exitCode;
		if (_server.IsRunning(exitCode))
			_server.Stop();
	
		_status = TTSStatus::Uninitialized;
	}

	bool TTSBackend_Win::Restart()
	{
		if (_status == TTSStatus::Uninitialized)
			return false;
		
		Shutdown();
		return Initialize();
	}

	bool TTSBackend_Win::CheckHealth()
	{
		if (_status == TTSStatus::Uninitialized)
			return false;

		int32_t exitCode = -255;
		if (_server.IsRunning(exitCode))
			return true; // Still running

		if (exitCode != -255)
			LogLn(std::format("audiocpp_server exited with code {}", exitCode));
		return false;
	}

	std::expected<TTSData, TTSError> TTSBackend_Win::SendRequest(TTSTask task, fig::string_view text, fig::string_view instructions)
	{
		if (not _http.IsConnected())
		{
			if (not _http.Connect(L"localhost", Constants::TTS::ServerPort))
				return std::unexpected(TTSError::Unavailable);
		}

		if (task == TTSTask::Speak)
		{
			fig::string request = std::format(R"({{
				"model": "{0}",
				"input": "{1}"
			}})", "chatterbox", text);

			if (auto response = _http.Post(L"/v1/audio/speech", request))
			{
				auto& data = response.value();
				return std::move(data);
			}
			else
				return std::unexpected(TTSError::Failed);
		}
		else if (task == TTSTask::Design)
		{
			fig::string strInstructions { instructions };
			escape_json_inplace(strInstructions);

			fig::string request = std::format(R"({{
				"model": "{0}",
				"input": "{1}",
				"instructions": "{2}"
			}})", "qwen3-design", text, strInstructions);

			if (auto response = _http.Post(L"/v1/audio/speech", request))
			{
				auto& data = response.value();
				return std::move(data);
			}
			else
				return std::unexpected(TTSError::Failed);
		}

		return std::unexpected(TTSError::Failed);
	}
}