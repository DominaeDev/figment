#include "llama.h"
#include "common.h"
#include "chat.h"
#include "LLMInstance.h"
#include "StringUtil.h"
#include "Utility.h"
#include "Constants.h"
#include <format>

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

size_t validate_utf8(const std::string& text)
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

size_t string_find_partial_stop(const std::string_view& str, const std::string_view& stop)
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

size_t find_one_of(const std::string& text, const std::vector<std::string>& words)
{
	size_t stop_pos = std::string::npos;

	for (const std::string& word : words)
	{
		size_t pos = text.find(word);
		if (pos != std::string::npos && (stop_pos == std::string::npos || pos < stop_pos))
			stop_pos = pos;
	}

	return stop_pos;
}

size_t find_stopping_strings(const std::string& text, const std::vector<std::string>& stop_words, const size_t last_token_size, bool is_full_stop)
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

static void get_tag_and_name(const string& text, string& tag, string& name)
{
	size_t pos_equals = text.find('=', 1);
	if (pos_equals == std::string::npos)
	{
		tag = trim(text.substr(1, text.length() - 2));
		name = "";
		return;
	}

	tag = trim(text.substr(1, pos_equals - 1));
	name = trim(text.substr(pos_equals + 1, text.length() - pos_equals - 2));
	string_replace_all(name, "\"", "");
}

static void __LoadModel(string filename, __LoadModelCallback onComplete)
{
	const int ngl = 99;
	const int n_ctx = 2048;

	// initialize the model
	llama_model_params model_params = llama_model_default_params();
	model_params.n_gpu_layers = ngl;
	model_params.use_mlock = true;
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

void process(string& partial, string str_token, bool* bWait, bool* bHalt)
{
	if (validate_utf8(partial) < partial.size()) // Incomplete utf-8 string
	{
		*bWait = true;
		*bHalt = false;
		return;
	}

	static std::vector<std::string> stop_words {
		"<|",
		"<end_of_turn",
		"<EOT>",
		"_<EOT>",
		"<s>",
		"</s>",
		"### ",
		"<｜",
//		"<|end",
//		"<｜end▁of▁sentence｜>",
	};

	static std::vector<std::string> opening_tags {
		std::format("<{0}=\"", Constants::DialogueTag),
		std::format("<{0}=\"", Constants::ActionTag),
		std::format("<{0}=\"", Constants::ThoughtTag),
		std::format("<{0}>", Constants::NarrationTag),
	};

	static std::vector<std::string> closing_tags {
		std::format("</{0}>", Constants::DialogueTag),
		std::format("</{0}>", Constants::ActionTag),
		std::format("</{0}>", Constants::ThoughtTag),
		std::format("</{0}>", Constants::NarrationTag),
	};
	static std::vector<std::string> formatting_tags;
	if (formatting_tags.empty())
	{
		formatting_tags.insert(std::end(formatting_tags), std::begin(opening_tags), std::end(opening_tags));
		formatting_tags.insert(std::end(formatting_tags), std::begin(closing_tags), std::end(closing_tags));
	}

	// Look for stop word - and halt
	size_t stop_pos = find_stopping_strings(partial, stop_words, str_token.size(), true);
	if (stop_pos != std::string::npos)
	{
		std::string stop_word = partial.substr(stop_pos);

		// Print to console
		partial = partial.erase(stop_pos); // Erase stop word
		*bHalt = true;
		*bWait = false;

		printf("%s[%s]", partial.c_str(), stop_word.c_str());
		fflush(stdout);
		return;
	}

	// Look for partial stop word - and wait
	stop_pos = find_stopping_strings(partial, stop_words, str_token.size(), false);
	if (stop_pos != std::string::npos)
	{
		*bHalt = false;
		*bWait = true;
		return;
	}

	// Look for formatting tags
	size_t fmt_pos = find_one_of(partial, opening_tags);
	if (fmt_pos != std::string::npos)
	{
		// Await end of tag '>', or beginning of a new tag '<' (indicating garbage from the model)
		if (partial.find_first_of("<>", fmt_pos + 1, 2) == std::string::npos)
		{
			*bHalt = false;
			*bWait = true;
			return;
		}

		int k = 0;
	}
	else
	{
		// Look for partial formatting tags - and wait
		fmt_pos = find_stopping_strings(partial, formatting_tags, str_token.size(), false);
		if (fmt_pos != std::string::npos)
		{
			*bHalt = false;
			*bWait = true;
			return;
		}
	}

	*bHalt = false;
	*bWait = false;
}

void LLMInstance::__Generate(string prompt, __PartialResultCallback onPartial, __GenerationCompleteCallback onComplete)
{
	std::string response;

	ModelState state = _atm_modelState.load();
	
	string userName = state.context_builder->participants[0].name;
	string botName = state.context_builder->participants[1].name;

	GenerationState genState;
	genState.messageId = ++_messageCounter;

	const int32_t maxCtx = llama_n_ctx(state.pCtx);
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

	// Prepare a batch for the prompt
	llama_batch batch = llama_batch_init(maxCtx, 0, 1);
	int32_t num_tokens = (int32_t)prompt_tokens.size();
	
	// Add tokens to batch
	for (int i = 0; i < num_tokens; ++i) {
		batch.token[i] = prompt_tokens[i];
		batch.pos[i] = i;  // Position in sequence
		batch.n_seq_id[i] = 1;  // This token belongs to 1 sequence
		batch.seq_id[i][0] = 0;  // Sequence ID 0
		batch.logits[i] = false;  // Don't need logits for most tokens
	}
	batch.logits[num_tokens - 1] = true;  // Only need logits for last token
	batch.n_tokens = num_tokens;

	llama_kv_self_clear(state.pCtx);

	llama_token new_token_id;
	std::string partial;
	int fail_safe = 0;
	while (fail_safe++ < 100)
	{
		bool next_token = true;

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
			partial.clear();
			break;
		}

		// convert the token to a string, print it and add it to the response
		std::string str_token = stringFromToken(state.pVocab, new_token_id);
		if (str_token.size() == 0)
			break; // Error

		partial += str_token;
		bool send = true;

		// check if there is incomplete UTF-8 character at the end
		bool bHalt = false;
		bool bWait = false;
		process(partial, str_token, &bWait, &bHalt);
		next_token &= !bHalt;
		send &= !bWait;

		if (_atm_bCancelGeneration.load())
			break; // Cancelled

		if (send)
		{
			// Clean up incomplete stop tokens
//			size_t stop_pos = partial.find("<|im");
//			if (stop_pos != std::string::npos)
//				partial = partial.erase(stop_pos); // Erase left-over

			string carryOver;
			string sendMsg = partial;
			bool bEndOfMessageType = false;

			// Check and erase formatting tags
			size_t fmt_start = partial.find('<');
			if (fmt_start != std::string::npos)
			{
				bool bRemove = false;
				size_t fmt_end = partial.find('>', fmt_start + 1);
				if (fmt_end != std::string::npos)
				{
					string tag, tagName;
					get_tag_and_name(partial.substr(fmt_start, fmt_end - fmt_start + 1), tag, tagName);

					if (tagName == userName) // Stop if talking/acting for the user
						break;

					if (tag == "/dlg" || tag == "/act" || tag == "/narration" || tag == "/thought")
					{
						carryOver = partial.substr(fmt_end + 1);
						partial.erase(fmt_end + 1);

						sendMsg.erase(fmt_start);
						bEndOfMessageType = true;
					}
					else
					{
						if (fmt_start > 0)
						{
							// Send remainder first
							carryOver = partial.substr(fmt_start);
							partial.erase(fmt_start);
							sendMsg = partial;
						}
						else
						{
							sendMsg.erase(fmt_start, fmt_end - fmt_start + 1);
							genState.currName = tagName;
							if (tag == "dlg")
								genState.msgType = MessageType::Dialogue;
							else if (tag == "act")
								genState.msgType = MessageType::Action;
							else if (tag == "narration")
								genState.msgType = MessageType::Narration;
							else if (tag == "thought")
								genState.msgType = MessageType::Thought;
						}
					}
				}
			}

			MessageType msgType = genState.msgType;
			if (msgType == MessageType::Undefined)
				msgType = MessageType::Dialogue;

			// Send piece
			if (partial.size() > 0)
			{
				_resultMutex.lock();
				_generatedText += partial;

				_resultQueue.push(MessagePiece {
					genState.messageId,
					genState.currName,
					sendMsg,
					msgType,
					bEndOfMessageType,
				});
				_resultMutex.unlock();
				response += partial;
			}

			partial = carryOver;
			if (bEndOfMessageType)
				genState.msgType = MessageType::Undefined;

			send = false;
			fail_safe = 0;
		}

		// Print to console
		printf("%s", str_token.c_str());
		fflush(stdout);

		// prepare the next batch with the sampled token
		if (!next_token)
			break; // TODO: Carry over?

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

bool LLMInstance::SendMessage(string name, string message, bool generate)
{
	if (!IsReady() || IsGenerating())
		return false;

	// Reset response
	_resultMutex.lock();
	_generatedText.clear();
	while (!_resultQueue.empty())
		_resultQueue.pop();
	_resultMutex.unlock();

	ModelState state = _atm_modelState.load();

	// add the user input to the message list and format it
	state.context_builder->messages.push_back(Message { Role::User, message });

	if (!generate)
		return true;

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

	uint32_t allocCtx = llama_n_ctx(state.pCtx);
	uint32_t usedCtx = llama_kv_self_used_cells(state.pCtx);

	_statusCallback(LLMStatus { _modelName, allocCtx, usedCtx });
}

bool LLMInstance::PollResponse(MessagePiece& piece)
{
	if (!_resultMutex.try_lock())
		return false;

	if (_resultQueue.empty())
	{
		_resultMutex.unlock();
		return false;
	}

	piece = _resultQueue.front();
	_resultQueue.pop();
	_resultMutex.unlock();
	return true;
}