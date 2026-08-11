#include <pch.h>
#include "tts/TTSBackend_Win.h"

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

//		const char* arguments[] = { "bin/audiocpp/audiocpp_server.exe", "--config", "bin/audiocpp/server.json", nullptr };

		if (auto started = _server.Start())
		{
			_status = TTSStatus::ServerStarted;
			return true;
		}
		else
			return false;
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

	std::expected<TTSData, TTSError> TTSBackend_Win::SendRequest(TTSTask task, fig::string_view text)
	{
		if (not _http.IsConnected())
		{
			if (not _http.Connect(L"localhost", 8080))
				return std::unexpected(TTSError::Unavailable);
		}

		fig::string request = std::format("{{ \"model\": \"{}\", \"input\": \"{}\" }}", "pocket-tts", text);
		if (auto response = _http.Post(L"/v1/audio/speech", request))
		{
			auto& data = response.value();
			return std::move(data);
		}
		else
			return std::unexpected(TTSError::Failed); //! @todo
	}
}