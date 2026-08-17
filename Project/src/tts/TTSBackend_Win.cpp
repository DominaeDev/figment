#include <pch.h>
#include "tts/TTSBackend_Win.h"
#include "io/FileUtility.h"

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

		AudioServerConfiguration serverConfig;
		serverConfig.backend = AudioServerConfiguration::Backend::CUDA;
		serverConfig.models = _models;

		if (auto started = _server.Start(serverConfig))
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

	std::expected<AudioData, TTSError> TTSBackend_Win::SendRequest(TTSTask task, fig::uuid modelId, fig::string_view text, fig::string_view instructions, TTSVoiceRef voiceRef)
	{
		if (not _http.IsConnected())
		{
			if (not _http.Connect(L"localhost", Constants::TTS::ServerPort))
				return std::unexpected(TTSError::Unavailable);
		}

		if (task == TTSTask::Speech)
		{
			string ref_text = voiceRef.referenceText;

			fig::string request;
			if (voiceRef.pData)
			{
				request = std::format(R"({{
					"model": "{0}",
					"input": "{1}",
					"voice_ref": {{ "type": "base64", "data": "{2}" }},
					"reference_text": "{3}"
				}})", (fig::string)modelId, text, voiceRef.pData->AsBase64(), ref_text);
			}
			else
			{
				request = std::format(R"({{
					"model": "{0}",
					"input": "{1}"
				}})", (fig::string)modelId, text);
			}

			if (auto response = _http.Post(L"/v1/audio/speech", request))
				return std::move(AudioData::FromBytes(std::move(response.value())));
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
			}})", (fig::string)modelId, text, strInstructions);

			if (auto response = _http.Post(L"/v1/audio/speech", request))
				return std::move(AudioData::FromBytes(std::move(response.value())));
			else
				return std::unexpected(TTSError::Failed);
		}
		else if (task == TTSTask::Unload)
		{
			if (not modelId.empty())
			{
				fig::string request = std::format(R"({{ "model_ids": ["{}"] }})", (fig::string)modelId);

				if (auto response = _http.Post(L"/v1/tasks/unload_models", request))
				{
					LogLn(std::format("Unloaded TTS model {}", (fig::string)modelId));
					return {};
				}
				else
					return std::unexpected(TTSError::Failed);
			}
			else
			{
				if (auto response = _http.Post(L"/v1/tasks/unload_all_models", ""))
				{
					LogLn("Unloaded all TTS models");
					return {};
				}
				else
					return std::unexpected(TTSError::Failed);
			}
		}

		return std::unexpected(TTSError::Failed);
	}
}