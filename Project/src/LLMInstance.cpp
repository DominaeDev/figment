#include "llama.h"
#include "common.h"
#include "chat.h"
#include "LLMInstance.h"
#include "StringUtil.h"
#include "Utility.h"

void ModelState::Release()
{
	if (pSampler)
		llama_sampler_free(pSampler);
	if (pCtx)
	{
		llama_kv_self_clear(pCtx);
		llama_free(pCtx);
	}
	if (pModel)
		llama_model_free(pModel);

	delete context_builder;
	pSampler = nullptr;
	pCtx = nullptr;
	pModel = nullptr;
	pVocab = nullptr;
	bReady = false;
}

LLMInstance::~LLMInstance()
{
	if (_workerThread.get() != nullptr && _workerThread.get()->joinable())
		_workerThread.get()->join();

	_statusCallback = nullptr;
	Shutdown();
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
	Halt();

	// Clear state and release
	auto state = _atm_modelState.exchange(ModelState());
	state.Release();

	ReportStatus();
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
	model_params.use_mlock = true;
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

	auto pCtxBuilder = new ContextBuilder();
	pCtxBuilder->system_prompt = LoadTextFile("characters/system.txt");
	pCtxBuilder->LoadBot("characters/character.xml");
	pCtxBuilder->LoadUser("characters/user.xml");

	state.context_builder = pCtxBuilder;
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
		[this, filename, onComplete](ModelState result)
	{
		if (result.bReady)
		{
			_atm_modelState.store(result);
			_modelName = get_filename(filename);
			onComplete(true);
			ReportStatus();
		}
		else
		{
			result.Release();
			_modelName.clear();
			onComplete(false);
			ReportStatus();
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

bool LLMInstance::IsGenerating() const
{
	return _atm_bGeneratingResponse.load();
}

bool LLMInstance::Resume()
{
	if (!IsReady() || IsGenerating())
		return false;

	return Generate("");
}

bool LLMInstance::Halt()
{
	if (!IsReady() || !IsGenerating())
		return false;

	_atm_bCancelGeneration.store(true);
	if (_workerThread.get() != nullptr && _workerThread.get()->joinable())
		_workerThread.get()->join();
	return true;
}

void LLMInstance::__Generate(string prompt, __PartialResultCallback onPartial, __GenerationCompleteCallback onComplete)
{
	std::string response;
	std::vector<std::string> stop_words {
		"<|",
		"<｜"
		"<end_of_turn",
//		"<|end",
		"<EOT>",
		"_<EOT>",
//		"<｜end▁of▁sentence｜>",
		"</s>",
		"### ",
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

	printf("\r\n");
	fflush(stdout);

	// prepare a batch for the prompt
	llama_batch batch = llama_batch_get_one(prompt_tokens.data(), (int32_t)prompt_tokens.size());
	llama_token new_token_id;
	std::string partial;
	bool running = true;
	while (running)
	{
		// check if we have enough space in the context to evaluate this batch
		int n_ctx = llama_n_ctx(state.pCtx);
		int n_ctx_used = llama_kv_self_used_cells(state.pCtx);
		if (n_ctx_used + batch.n_tokens > n_ctx)
		{
			fprintf(stderr, "context size exceeded\n");
			onComplete(2, response);
			return;
		}

		if (batch.n_tokens > 0 && llama_decode(state.pCtx, batch))
		{
			fprintf(stderr, "failed to decode\n");
			onComplete(3, response);
			return;
		}

		// sample the next token
		new_token_id = llama_sampler_sample(state.pSampler, state.pCtx, -1);

		// is it an end of generation?
		if (llama_vocab_is_eog(state.pVocab, new_token_id))
		{
			running = false; // Halt
			if (partial.size() > 0)
				partial.clear();
		}

		// convert the token to a string, print it and add it to the response
		std::string str_token = stringFromToken(state.pVocab, new_token_id);
		if (str_token.size() == 0)
			break; // Error

		partial += str_token;
		bool send = true;
		bool next_token = true;

		// check if there is incomplete UTF-8 character at the end
		bool incompleteUtf8 = validate_utf8(partial) < partial.size();

		if (incompleteUtf8)
		{
			// Wait for next token
			send = false;
		}
		else
		{
			// Process response
			size_t stop_pos = find_stopping_strings(partial, stop_words, str_token.size(), true);
			if (stop_pos != std::string::npos)
			{
				std::string stop_word = partial.substr(stop_pos);

				// Print to console
				partial = partial.erase(stop_pos); // Erase stop word
				running = false; // Halt
				next_token = false;

				send = partial.size() > 0;
				if (send)
				{
					int k = 0;
				}
				printf("%s[%s]", partial.c_str(), stop_word.c_str());
				fflush(stdout);
			}
			else
			{
				// Look for partial stop word - and wait
				stop_pos = find_stopping_strings(partial, stop_words, str_token.size(), false);
				if (stop_pos != std::string::npos)
				{
					// Wait for more
					send = false;
					next_token = true;
				}
			}
		}

		if (_atm_bCancelGeneration.load())
			break; // Cancelled

		if (send)
		{
			// Print to console
			printf("%s", str_token.c_str());
			fflush(stdout);

			// Clean up incomplete stop tokens
			size_t stop_pos = partial.find("<|im");
			if (stop_pos != std::string::npos)
				partial = partial.erase(stop_pos); // Erase left-over

			// Send piece
			_mutex_generatedText.lock();
			_generatedText += partial;
			_lastResponse += partial;
			_mutex_generatedText.unlock();
			response += partial;

			onPartial(__PartialResult { partial, response});

			partial.clear();
			send = false;
		}

		// prepare the next batch with the sampled token
		if (!next_token)
			break;
		
//		common_batch_add(batch, new_token_id, 0, llama_tokens {}, false);
		batch = llama_batch_get_one(&new_token_id, 1);
	}

	onComplete(0, response);
};

bool LLMInstance::Generate(const string& prompt)
{
	if (!IsReady() || IsGenerating())
		return false;

	if (_workerThread.get() != nullptr && _workerThread.get()->joinable())
		_workerThread.get()->join();

	_atm_bCancelGeneration.store(false);
	_atm_bGeneratingResponse.store(true);

	auto pThread = new std::thread(&LLMInstance::__Generate, this, prompt, 
		[](__PartialResult partial) {
			// ...
		},
		[this](int error, string response) {
			// ...
			_atm_bGeneratingResponse.store(false);
			ModelState state = _atm_modelState.load();
			state.context_builder->messages.push_back({ Role::Bot, response });
			ReportStatus();
		});

	_workerThread.reset(pThread);
	return true;
};

bool LLMInstance::SendMessage(string name, string message)
{
	if (!IsReady() || IsGenerating())
		return false;

	// Reset response
	_mutex_generatedText.lock();
	_generatedText.clear();
	_lastResponse.clear();
	_mutex_generatedText.unlock();

	ModelState state = _atm_modelState.load();

	// add the user input to the message list and format it
	state.context_builder->messages.push_back(Message { Role::User, message });

	std::vector<llama_chat_message> messages = state.context_builder->GetMessages();

	std::vector<char> formatted(llama_n_ctx(state.pCtx));
	int prev_len = 0;

	/*int32_t nMeta = llama_model_meta_count(state.pModel);
	char* metaKey = new char[512];
	char* metaValue = new char[2048];
	for (int32_t i = 0; i < nMeta; ++i)
	{
		if (!llama_model_meta_key_by_index(state.pModel, i, metaKey, 512))
			continue;
		if (llama_model_meta_val_str(state.pModel, metaKey, metaValue, 2048))
			printf("%s = %s\r\n", metaKey, metaValue);
	}*/

//	const char* tmpl = llama_model_chat_template(state.pModel, nullptr);
	const char* tmpl = "chatml";

//	tmpl = "mistral-v7-tekken";
//	tmpl = "chatml";
//	tmpl = "llama2";
//	tmpl = "llama3";
//	tmpl = "command-r";
//	tmpl = "gemma";
//	tmpl = "vicuna";
//	tmpl = "deepseek3";

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
	
	// Free
	for (auto& m : messages)
	{
		free(const_cast<char*>(m.role));
		free(const_cast<char*>(m.content));
	}

	// generate a response
	return Generate(prompt);
}

bool LLMInstance::TryGetResponse(string& result)
{
	if (!_mutex_generatedText.try_lock())
		return false;

	result = _generatedText;
	_generatedText.clear();
	_mutex_generatedText.unlock();
	return result.size() > 0;
}

void LLMInstance::ReportStatus()
{
	if (!_statusCallback)
		return;

	ModelState state = _atm_modelState.load();

	if (!state.pModel || !state.pCtx)
	{
		_statusCallback(LLMStatus());
		return;
	}

	uint32_t allocCtx = llama_n_batch(state.pCtx);
	uint32_t usedCtx = llama_kv_self_used_cells(state.pCtx);

	_statusCallback(LLMStatus { _modelName, allocCtx, usedCtx });
}