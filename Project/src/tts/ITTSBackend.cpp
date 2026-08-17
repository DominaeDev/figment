#include <pch.h>
#include "tts/ITTSBackend.h"

namespace fig::tts
{
	ITTSBackend::ITTSBackend()
	{
		LoadModelConfigurations();

		// Start worker thread
		_worker = std::jthread(std::bind_front(&ITTSBackend::__Worker, this));
	}

	ITTSBackend::~ITTSBackend()
	{
		// Shut down worker thread
		_worker.request_stop();
		_pending_cv.notify_all();
	}

	void ITTSBackend::__Worker(std::stop_token stop)
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

	bool ITTSBackend::IsAsyncRequestAlive(const PendingRequest& request) const
	{
		std::scoped_lock lock(_active_mutex);
		auto it = _active_promises.find(request.id);
		if (it == _active_promises.cend())
			return false;
		return it != _active_promises.end();
	}

	std::optional<TTSResult> ITTSBackend::EnqueueTask(TTSTask task, fig::uuid modelId, fig::uuid characterId, fig::string_view text, fig::string_view instructions)
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

	std::expected<std::vector<TTSResult>, TTSError> ITTSBackend::Speak(fig::uuid characterId, fig::string_view text, bool split)
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

	std::expected<TTSResult, TTSError> ITTSBackend::Design(fig::string_view text, fig::string_view instruct)
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

	void ITTSBackend::UnloadAllModels()
	{
		if (_status == TTSStatus::Uninitialized)
			return;

		auto discard = EnqueueTask(TTSTask::Unload, {}, {}, {}, {});
	}

	void ITTSBackend::UnloadSpeechModels()
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

	void ITTSBackend::UnloadDesignModels()
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

	void ITTSBackend::LoadModelConfigurations()
	{
		_models.LoadFromXml(fig::path { "tts/models.xml" });

		// Remove all but installed models
		for (auto& model : _models.models)
			std::erase_if(model.variants, [](auto&& m) { return not std::filesystem::exists(fig::path { Constants::Paths::TTSModels } / fig::path { m.filename }); });
		std::erase_if(_models.models, [](auto&& m) { return m.variants.empty(); });
	}
}