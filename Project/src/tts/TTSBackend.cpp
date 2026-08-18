#include <pch.h>
#include "tts/TTSBackend.h"

#if defined(_WIN32)
#include "tts/AudioServerProcess_Win32.h"
#include "tts/HttpClient_Win32.h"
#else
#include "tts/AudioServerProcess_SDL.h"
#include "tts/IHttpClient.h"
#endif

namespace fig::tts
{
	TTSBackend::TTSBackend()
	{
#if defined(_WIN32)
		_pServer = std::make_unique<AudioServerProcess_Win32>();
		_pHttp = std::make_unique<HttpClient_Win32>();
#else
		_pServer = std::make_unique<AudioServerProcess_SDL>();
		_pHttp = std::make_unique<HttpClient_Dummy>(); //! @todo
#endif

		LoadModelConfigurations();

		// Start worker thread
		_worker = std::jthread(std::bind_front(&TTSBackend::__Worker, this));
	}

	TTSBackend::~TTSBackend()
	{
		Shutdown();

		// Shut down worker thread
		_worker.request_stop();
		_pending_cv.notify_all();
	}

	void TTSBackend::__Worker(std::stop_token stop)
	{
		PendingRequest request;
		while (not stop.stop_requested())
		{
			// Block until next task
			{
				std::unique_lock lock(_pending_mutex);
				_pending_cv.wait(lock, [&] {
					return !_pending.empty() || stop.stop_requested();
				});
				if (stop.stop_requested())
					break; // Stop

				request = std::move(const_cast<PendingRequest&>(_pending.front()));
				_pending.pop();
			}

			if (not IsAsyncRequestAlive(request))
			{
				request.promise->set_value(std::unexpected(TTSError::Canceled));
				continue;
			}

			// Do work
			auto result = SendRequest(request.task, request.modelId, request.text, request.instructions, request.voiceReference);
			
			if (IsAsyncRequestAlive(request))
			{
				std::scoped_lock<std::mutex> lock(_active_mutex);
				_active_promises.erase(request.id);
			}
			else
			{
				// Canceled
				request.promise->set_value(std::unexpected(TTSError::Canceled));
				continue;
			}

			if (result.has_value())
				request.promise->set_value(std::move(result.value()));
			else
				request.promise->set_value(std::unexpected(result.error()));
		}
	}

	bool TTSBackend::IsAsyncRequestAlive(const PendingRequest& request) const
	{
		std::scoped_lock lock(_active_mutex);
		auto it = _active_promises.find(request.id);
		if (it == _active_promises.cend())
			return false;
		return it != _active_promises.end();
	}

	std::optional<TTSResult> TTSBackend::EnqueueTask(TTSTask task, fig::uuid modelId, fig::uuid characterId, fig::string_view text, fig::string_view instructions)
	{
		if (_status == TTSStatus::Uninitialized)
		{
			if (not Initialize())
				return std::nullopt; // Give up
		}

		const uint64_t id = _next_id.fetch_add(1, std::memory_order_relaxed);

		TTSVoiceRef voiceRef {};
		if (not characterId.empty())
		{
			if (auto try_voice = Global::GetUserContent().GetVoiceForCharacter(characterId))
			{
				voiceRef.pData = &(*try_voice).voicePrint.audioData;
				voiceRef.referenceText = (*try_voice).voicePrint.referenceText;
			}
		}

		// Create the promise
		auto promise = std::make_unique<TTSPromise>();
		auto future = promise->get_future();
		auto promise_ptr = promise.get();

		{	// Store promise
			std::scoped_lock lock(_active_mutex);
			_active_promises[id] = promise_ptr;
		}

		{	// Enqueue request
			std::scoped_lock lock(_pending_mutex);
			_pending.push(PendingRequest {
				.id = id,
				.task = task,
				.modelId = modelId,
				.text = fig::string { text },
				.instructions = fig::string { instructions },
				.voiceReference = voiceRef,
				.promise = std::move(promise),
			});
		}
		_pending_cv.notify_one();

		return TTSResult {
			.id = id,
			.task = task,
			.future = std::move(future),
		};
	}

	std::expected<std::vector<TTSResult>, TTSError> TTSBackend::Speak(fig::uuid characterId, fig::string_view text, bool split)
	{
		text = Undialogue(text);
		text = Unaction(text);
		text = Unnarration(text);

		fig::string content { text };
		escape_json_inplace(content);

		fig::uuid modelId = Global::GetUserSettings().GetUUID(fig::io::UserSetting::TTS::TTSModel);

		std::vector<TTSResult> results;
		if (split)
		{
			auto sentences = split_sentences(content, true);
			
			// Chunk shorter sentences together
			constexpr size_t MinSentenceLength = 60;

			std::vector<fig::string> phrases;
			std::string scratch;
			for (auto& sentence : sentences)
			{
				if (not scratch.empty())
				{
					scratch.append(" ");
					scratch.append(sentence);
				}
				else
					scratch = sentence;

				if (scratch.length() >= MinSentenceLength)
				{
					phrases.push_back(scratch);
					scratch.clear();
				}
			}

			if (not scratch.empty())
				phrases.push_back(scratch);

			for (auto& phrase : phrases)
			{
				if (auto task = EnqueueTask(fig::tts::TTSTask::Speech, modelId, characterId, phrase))
					results.emplace_back(std::move(task).value());
				else
					return std::unexpected(TTSError::Unavailable);
			}

		}
		else if (not empty_or_whitespace(content)) // Don't split
		{
			if (auto task = EnqueueTask(fig::tts::TTSTask::Speech, modelId, characterId, content))
				results.emplace_back(std::move(task).value());
			else
				return std::unexpected(TTSError::Unavailable);
		}

		if (not results.empty())
			return results;
		return std::unexpected(TTSError::Failed);
	}

	std::expected<TTSResult, TTSError> TTSBackend::Design(fig::string_view text, fig::string_view instruct)
	{
		instruct = trim(instruct);
		if (instruct.empty())
			return std::unexpected(TTSError::Failed);

		fig::uuid modelId = Global::GetUserSettings().GetUUID(fig::io::UserSetting::TTS::DesignModel);

		if (auto task = EnqueueTask(fig::tts::TTSTask::Design, modelId, fig::uuid {}, fig::string { text }, fig::string { instruct }))
			return std::move(task).value();
		else
			return std::unexpected(TTSError::Unavailable);
	}

	void TTSBackend::UnloadAllModels()
	{
		if (_status == TTSStatus::Uninitialized)
			return;

		auto discard = EnqueueTask(TTSTask::Unload, {}, {}, {}, {});
	}

	void TTSBackend::UnloadSpeechModels()
	{
		if (_status == TTSStatus::Uninitialized)
			return;

		for (auto& model : _models.models)
		{
			if (model.task.task != TTSTask::Speech)
				continue;

			for (auto& variant : model.variants)
				auto discard = EnqueueTask(TTSTask::Unload, variant.id, {}, {}, {});
		}
	}

	void TTSBackend::UnloadDesignModels()
	{
		if (_status == TTSStatus::Uninitialized)
			return;

		for (auto& model : _models.models)
		{
			if (model.task.task != TTSTask::Design)
				continue;

			for (auto& variant : model.variants)
				auto discard = EnqueueTask(TTSTask::Unload, variant.id, {}, {}, {});
		}
	}

	void TTSBackend::LoadModelConfigurations()
	{
		_models.LoadFromXml(fig::path { "tts/models.xml" });

		// Remove all but installed models
		for (auto& model : _models.models)
			std::erase_if(model.variants, [](auto&& m) { return not std::filesystem::exists(fig::path { Constants::Paths::TTSModels } / fig::path { m.filename }); });
		std::erase_if(_models.models, [](auto&& m) { return m.variants.empty(); });
	}

	bool TTSBackend::Initialize()
	{
		using namespace fig::gui;

		if (_status != TTSStatus::Uninitialized)
		{
			if (CheckHealth())
				return true; // Already initialized
			Shutdown(); // Restart
		}

		AudioServerConfiguration serverConfig;
		serverConfig.backend = AudioServerConfiguration::Backend::CUDA;
		serverConfig.models = _models;

		if (auto started = _pServer->Start(serverConfig))
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

	void TTSBackend::Shutdown()
	{
		// Shut down server
		int32_t exitCode;
		if (_pServer->IsRunning(exitCode))
			_pServer->Stop();

		_status = TTSStatus::Uninitialized;
	}

	bool TTSBackend::Restart()
	{
		if (_status == TTSStatus::Uninitialized)
			return false;

		Shutdown();
		return Initialize();
	}

	bool TTSBackend::CheckHealth()
	{
		if (_status == TTSStatus::Uninitialized)
			return false;

		int32_t exitCode = -255;
		if (_pServer->IsRunning(exitCode))
			return true; // Still running

		if (exitCode != -255)
			LogLn(std::format("audiocpp_server exited with code {}", exitCode));
		return false;
	}

	std::expected<AudioData, TTSError> TTSBackend::SendRequest(TTSTask task, fig::uuid modelId, fig::string_view text, fig::string_view instructions, TTSVoiceRef voiceRef)
	{
		if (not _pHttp->IsConnected())
		{
			if (not _pHttp->Connect("localhost", Constants::TTS::ServerPort))
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

			if (auto response = _pHttp->Post("/v1/audio/speech", request))
				return std::move(AudioData::FromBytes(std::move(response.payload)));
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

			if (auto response = _pHttp->Post("/v1/audio/speech", request))
				return std::move(AudioData::FromBytes(std::move(response.payload)));
			else
				return std::unexpected(TTSError::Failed);
		}
		else if (task == TTSTask::Unload)
		{
			if (not modelId.empty())
			{
				fig::string request = std::format(R"({{ "model_ids": ["{}"] }})", (fig::string)modelId);

				if (auto response = _pHttp->Post("/v1/tasks/unload_models", request))
				{
					LogLn(std::format("Unloaded TTS model {}", (fig::string)modelId));
					return {};
				}
				else
					return std::unexpected(TTSError::Failed);
			}
			else
			{
				if (auto response = _pHttp->Post("/v1/tasks/unload_all_models", ""))
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