#include "llama.h"
#include "LLMInstance.h"
#include <vector>

#pragma comment(lib, "ggml-base.lib")

void ModelState::Release()
{
	llama_sampler_free(pSampler);
	llama_free(pCtx);
	llama_model_free(pModel);
	pSampler = nullptr;
	pCtx = nullptr;
	pModel = nullptr;
	pVocab = nullptr;
	bReady = false;
}

void LLMInstance::Initialize()
{
	// only print errors
	llama_log_set([](enum ggml_log_level level, const char* text, void* /* user_data */) {
		if (level >= GGML_LOG_LEVEL_ERROR)
		{
			fprintf(stderr, "%s", text);
		}
	}, nullptr);

	// load dynamic backends
	ggml_backend_load_all();

	_atm_bCancelGeneration.store(false);
	_atm_bGeneratingResponse.store(false);
	_atm_modelState.store(ModelState());
}

void LLMInstance::Shutdown()
{
	if (_workerThread.get() != nullptr && _workerThread.get()->joinable())
		_workerThread.get()->join();

	// Clear state and release
	auto state = _atm_modelState.exchange(ModelState());
	state.Release();
}

typedef std::function<void(ModelState)> __LoadModelCallback;
static LoadModelProgressCallback __LoadModelProgressCallback = nullptr;

static void OnLoadModelProgress(float progress, void* user_data)
{
	if (__LoadModelProgressCallback)
		__LoadModelProgressCallback(static_cast<int>(progress * 100.0f));
}

static std::string stringFromToken(const llama_vocab* pVocab, llama_token token)
{
	// convert the token to a string, print it and add it to the response
	char buf[256];
	int n = llama_token_to_piece(pVocab, token, buf, sizeof(buf), 0, true);
	if (n < 0)
		return "";

	return std::string(buf, n);
}

static void __LoadModel(string filename, __LoadModelCallback onComplete)
{
	int ngl = 99;
	int n_ctx = 2048;

	// initialize the model
	llama_model_params model_params = llama_model_default_params();
	model_params.n_gpu_layers = ngl;
	model_params.progress_callback = (llama_progress_callback)&OnLoadModelProgress;

	ModelState state;
	state.pModel = llama_model_load_from_file(filename.c_str(), model_params);
	if (!state.pModel)
	{
		fprintf(stderr, "%s: error: unable to load model\n", __func__);
		onComplete(state);
		return;
	}

	state.pVocab = llama_model_get_vocab(state.pModel);

	// initialize the context
	llama_context_params ctx_params = llama_context_default_params();
	ctx_params.n_ctx = n_ctx;
	ctx_params.n_batch = n_ctx;

	state.pCtx = llama_init_from_model(state.pModel, ctx_params);
	if (!state.pCtx)
	{
		fprintf(stderr, "%s: error: failed to create the llama_context\n", __func__);
		onComplete(state);
		return;
	}

	// initialize the sampler
	llama_sampler_chain_params sampler_params = llama_sampler_chain_default_params();
	state.pSampler = llama_sampler_chain_init(sampler_params);

	llama_sampler_chain_add(state.pSampler, llama_sampler_init_min_p(0.05f, 1));			// Min P
	llama_sampler_chain_add(state.pSampler, llama_sampler_init_temp(0.8f));					// Temperature
	llama_sampler_chain_add(state.pSampler, llama_sampler_init_dist(LLAMA_DEFAULT_SEED));	// Seed

	state.bReady = true;
	onComplete(state);
}

bool LLMInstance::LoadModelAsync(string filename, LoadModelProgressCallback onProgress, LoadModelCallback onComplete)
{
	if (IsReady())
		return false; // Already loaded

	if (_workerThread.get() != nullptr && _workerThread.get()->joinable())
		_workerThread.get()->join();

	_bLoadingModel = true;

	__LoadModelProgressCallback = onProgress;

	auto pThread = new std::thread(__LoadModel,
		filename,
		[this, onComplete](ModelState result)
	{
		if (result.bReady)
		{
			_atm_modelState.store(result);
			onComplete(true);
		}
		else
		{
			result.Release();
			onComplete(false);
		}
	});

	_workerThread.reset(pThread);

	return true;
}

bool LLMInstance::IsReady() const
{
	ModelState state = _atm_modelState.load();
	return state.bReady && state.pModel;
}

bool LLMInstance::Stop()
{
	if (!IsReady() || !_atm_bGeneratingResponse.load())
		return false;

	ModelState state = _atm_modelState.load();
	_atm_bCancelGeneration.store(true);
	return true;
}

static size_t validate_utf8(const std::string& text)
{
	size_t len = text.size();
	if (len == 0) return 0;

	// Check the last few bytes to see if a multi-byte character is cut off
	for (size_t i = 1; i <= 4 && i <= len; ++i)
	{
		unsigned char c = text[len - i];
		// Check for start of a multi-byte sequence from the end
		if ((c & 0xE0) == 0xC0)
		{
			// 2-byte character start: 110xxxxx
			// Needs at least 2 bytes
			if (i < 2) return len - i;
		}
		else if ((c & 0xF0) == 0xE0)
		{
			// 3-byte character start: 1110xxxx
			// Needs at least 3 bytes
			if (i < 3) return len - i;
		}
		else if ((c & 0xF8) == 0xF0)
		{
			// 4-byte character start: 11110xxx
			// Needs at least 4 bytes
			if (i < 4) return len - i;
		}
	}

	// If no cut-off multi-byte character is found, return full length
	return len;
}

static bool string_ends_with(const std::string_view& str, const std::string_view& suffix)
{
	return str.size() >= suffix.size() && str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

static size_t string_find_partial_stop(const std::string_view& str, const std::string_view& stop)
{
	if (!str.empty() && !stop.empty())
	{
		const char text_last_char = str.back();
		for (int64_t char_index = stop.size() - 1; char_index >= 0; char_index--)
		{
			if (stop[char_index] == text_last_char)
			{
				const auto current_partial = stop.substr(0, char_index + 1);
				if (string_ends_with(str, current_partial))
				{
					return str.size() - char_index - 1;
				}
			}
		}
	}

	return std::string::npos;
}

static size_t find_stopping_strings(const std::string& text, const std::vector<std::string>& stop_words, const size_t last_token_size, bool is_full_stop)
{
	size_t stop_pos = std::string::npos;

	for (const std::string& word : stop_words)
	{
		size_t pos;

		if (is_full_stop)
		{
			const size_t tmp = word.size() + last_token_size;
			const size_t from_pos = text.size() > tmp ? text.size() - tmp : 0;

			pos = text.find(word, from_pos);
		}
		else
		{
			// otherwise, partial stop
			pos = string_find_partial_stop(text, word);
		}

		if (pos != std::string::npos && (stop_pos == std::string::npos || pos < stop_pos))
		{
			stop_pos = pos;
		}
	}

	return stop_pos;
}

void LLMInstance::__Generate(const string& prompt, __PartialResultCallback onPartial, __GenerationCompleteCallback onComplete)
{
	std::string response;
	std::vector<std::string> stop_words {
		"<|im_end|>"
	};

	ModelState state = _atm_modelState.load();

	const bool is_first = llama_kv_self_used_cells(state.pCtx) == 0;

	// tokenize the prompt
	const int32_t n_prompt_tokens = -llama_tokenize(state.pVocab, prompt.c_str(), (int32_t)prompt.size(), NULL, 0, is_first, true);
	std::vector<llama_token> prompt_tokens(n_prompt_tokens);
	if (llama_tokenize(state.pVocab, prompt.c_str(), (int32_t)prompt.size(), prompt_tokens.data(), (int32_t)prompt_tokens.size(), is_first, true) < 0)
	{
		onComplete(1, response);
		return;
	}

	// prepare a batch for the prompt
	llama_batch batch = llama_batch_get_one(prompt_tokens.data(), (int32_t)prompt_tokens.size());
	llama_token new_token_id;
	std::string partial;
	bool running = true;
	while (running)
	{
		if (_atm_bCancelGeneration.load())
			break; // Cancelled

		// check if we have enough space in the context to evaluate this batch
		int n_ctx = llama_n_ctx(state.pCtx);
		int n_ctx_used = llama_kv_self_used_cells(state.pCtx);
		if (n_ctx_used + batch.n_tokens > n_ctx)
		{
			fprintf(stderr, "context size exceeded\n");
			onComplete(2, response);
			return;
		}

		if (llama_decode(state.pCtx, batch))
		{
			GGML_ABORT("failed to decode\n");
		}

		// sample the next token
		new_token_id = llama_sampler_sample(state.pSampler, state.pCtx, -1);

		// is it an end of generation?
		if (llama_vocab_is_eog(state.pVocab, new_token_id))
			break; // Halt

		// convert the token to a string, print it and add it to the response
		std::string piece = stringFromToken(state.pVocab, new_token_id);
		if (piece.size() == 0)
			break; // Error

		// Print to console
		printf("%s", piece.c_str());
		fflush(stdout);

		// check if there is incomplete UTF-8 character at the end
		bool incomplete = validate_utf8(response) < response.size();
		bool send = true;

		if (!incomplete)
		{
			// Process response
			partial += piece;

			if (partial.find('<') != std::string::npos)
			{
				int k = 0;
			}

			size_t stop_pos = find_stopping_strings(partial, stop_words, piece.size(), true);
			if (stop_pos != std::string::npos)
			{
				partial = piece.substr(0, stop_pos); // Erase stop word
				running = false; // Halt
			}
			else
			{
				stop_pos = find_stopping_strings(partial, stop_words, piece.size(), false);
				if (stop_pos != std::string::npos)
				{
					// Wait and see
					send = false;
				}
			}
		}
		else
		{
			partial += piece;
		}

		if (send)
		{
			// Send piece
			_mutex_generatedText.lock();
			_generatedText += piece;
			_lastResponse += piece;
			_mutex_generatedText.unlock();
			response += piece;
			partial.clear();
			send = false;

			if (onPartial)
				onPartial(__PartialResult { response, piece });
		}

		// prepare the next batch with the sampled token
		batch = llama_batch_get_one(&new_token_id, 1);
	}

	onComplete(0, response);
};

bool LLMInstance::Generate(const string& prompt)
{
	if (!IsReady() || _atm_bGeneratingResponse.load())
		return false;

	if (_workerThread.get() != nullptr && _workerThread.get()->joinable())
		_workerThread.get()->join();

	_atm_bGeneratingResponse.store(true);

	auto pThread = new std::thread(&LLMInstance::__Generate, this, prompt, 
		[](__PartialResult partial) {
			// ...
		},
		[this](int error, string response) {
			// ...
			_atm_bGeneratingResponse.store(false);
		});

	_workerThread.reset(pThread);
	return true;
/*

	if (!IsReady())
		return false;

	std::string response;

	ModelState state = _atm_modelState.load();

	const bool is_first = llama_kv_self_used_cells(state.pCtx) == 0;

	// tokenize the prompt
	const int32_t n_prompt_tokens = -llama_tokenize(state.pVocab, prompt.c_str(), (int32_t)prompt.size(), NULL, 0, is_first, true);
	std::vector<llama_token> prompt_tokens(n_prompt_tokens);
	if (llama_tokenize(state.pVocab, prompt.c_str(), (int32_t)prompt.size(), prompt_tokens.data(), (int32_t)prompt_tokens.size(), is_first, true) < 0)
	{
		GGML_ABORT("failed to tokenize the prompt\n");
		return false;
	}

	// prepare a batch for the prompt
	llama_batch batch = llama_batch_get_one(prompt_tokens.data(), (int32_t)prompt_tokens.size());
	llama_token new_token_id;
	while (true)
	{
		// check if we have enough space in the context to evaluate this batch
		int n_ctx = llama_n_ctx(state.pCtx);
		int n_ctx_used = llama_kv_self_used_cells(state.pCtx);
		if (n_ctx_used + batch.n_tokens > n_ctx)
		{
			fprintf(stderr, "context size exceeded\n");
			return false;
		}

		if (llama_decode(state.pCtx, batch))
		{
			GGML_ABORT("failed to decode\n");
		}

		// sample the next token
		new_token_id = llama_sampler_sample(state.pSampler, state.pCtx, -1);

		// is it an end of generation?
		if (llama_vocab_is_eog(state.pVocab, new_token_id))
			break;

		// convert the token to a string, print it and add it to the response
		std::string piece = stringFromToken(state.pVocab, new_token_id);
		if (piece.size() == 0)
			break; // Error

		printf("%s", piece.c_str());
		fflush(stdout);

		_mutex_generatedText.lock();
		_generatedText += piece;
		_mutex_generatedText.unlock();

		response += piece;

		if (_atm_bCancelGeneration.load())
			break; // Cancelled

		// prepare the next batch with the sampled token
		batch = llama_batch_get_one(&new_token_id, 1);
	}

	outResponse = response;
	return true;*/
};

bool LLMInstance::SendMessage(string name, string message)
{
	if (!IsReady() || _atm_bGeneratingResponse.load())
		return false;

	// Reset response
	_mutex_generatedText.lock();
	_generatedText.clear();
	_lastResponse.clear();
	_mutex_generatedText.unlock();

	ModelState state = _atm_modelState.load();

	std::vector<llama_chat_message> messages;
	std::vector<char> formatted(llama_n_ctx(state.pCtx));
	int prev_len = 0;

	const char* tmpl = llama_model_chat_template(state.pModel, /* name */ nullptr);

	// add the user input to the message list and format it
	messages.push_back({ _strdup(name.c_str()), _strdup(message.c_str()) });

	int new_len = llama_chat_apply_template(tmpl, messages.data(), (int32_t)messages.size(), true, formatted.data(), (int32_t)formatted.size());
	if (new_len > (int)formatted.size())
	{
		formatted.resize(new_len);
		new_len = llama_chat_apply_template(tmpl, messages.data(), (int32_t)messages.size(), true, formatted.data(), (int32_t)formatted.size());
	}
	if (new_len < 0)
	{
		fprintf(stderr, "failed to apply the chat template\n");
		return false;
	}

	// remove previous messages to obtain the prompt to generate the response
	std::string prompt(formatted.begin() + prev_len, formatted.begin() + new_len);

	// generate a response
	return Generate(prompt);

	/*
	// add the response to the messages
	messages.push_back({ "assistant", _strdup(outResponse.c_str()) });
	prev_len = llama_chat_apply_template(tmpl, messages.data(), messages.size(), false, nullptr, 0);
	if (prev_len < 0)
	{
		fprintf(stderr, "failed to apply the chat template\n");
		return false;
	}
	*/
}

bool LLMInstance::TryGetResponse(string& result)
{
	if (!_atm_bGeneratingResponse.load())
		return false;
	if (!_mutex_generatedText.try_lock())
		return false;

	result = _generatedText;
	_generatedText.clear();
	_mutex_generatedText.unlock();
	return true;
}