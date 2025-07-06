#include "LLMInstance.h"
#include "common.h"
#include "StringUtil.h"
#include "Utility.h"
#include "Constants.h"
#include <format>
#include <algorithm>

std::vector<llama_token> __tokenize(llama_model* pModel, string prompt, bool bAddBOS);
bool __init_batch(llama_model* pModel, llama_context* pCtx, string prompt, llama_batch& out_pBatch);

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

	pSampler = nullptr;
	pCtx = nullptr;
	pModel = nullptr;
	bReady = false;
}

LLMInstance::LLMInstance()
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

LLMInstance::~LLMInstance()
{
	if (_workerThread.get() != nullptr && _workerThread.get()->joinable())
	{
		printf(">> Releasing. Waiting on worker thread ");
		fflush(stdout);
		_workerThread.get()->join();
		printf(">> Done!\r\n");
	}

	_statusCallback = nullptr;
	Shutdown();
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

static void apply_names(string& prompt, string userName, string botName)
{
	replace_all(prompt, "{{user}}", userName);
	replace_all(prompt, "{{char}}", botName);
}

static string apply_chat_template(std::vector<Message> in_messages, llama_context* pCtx, bool add_assistant)
{
	int prev_len = 0;

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

	static const char* SYSTEM_NAME = "system";
	static const char* USER_NAME = "{{user}}";
	static const char* BOT_NAME = "{{char}}";

	std::vector<llama_chat_message> llama_msgs(in_messages.size());
	for (int i = 0; i < in_messages.size(); ++i)
	{
		auto& msg = in_messages[i];
		if (msg.role == Role::System)
		{
			llama_msgs[i] = llama_chat_message { SYSTEM_NAME, msg.content.c_str() };
		}
		else
		{
			llama_msgs[i] = llama_chat_message { 
				msg.name.empty() ? (msg.role == Role::User ? USER_NAME : BOT_NAME) : msg.name.c_str(), 
				msg.content.c_str() 
			};
		}
	}

	std::vector<char> formatted(llama_n_ctx(pCtx));
	int new_len = llama_chat_apply_template(tmpl, llama_msgs.data(), (int32_t)llama_msgs.size(), add_assistant, formatted.data(), (int32_t)formatted.size());
	if (new_len > (int)formatted.size())
	{
		formatted.resize(new_len);
		new_len = llama_chat_apply_template(tmpl, llama_msgs.data(), (int32_t)llama_msgs.size(), add_assistant, formatted.data(), (int32_t)formatted.size());
	}

	if (new_len < 0)
	{
		fprintf(stderr, "failed to apply the chat template\n");
		return "";
	}

	// remove previous messages to obtain the prompt to generate the response
	string prompt = string(formatted.begin() + prev_len, formatted.begin() + new_len);

	return prompt;
}

static string apply_chat_template(Message msg, llama_context* pCtx, bool add_assistant)
{
	return apply_chat_template(std::vector<Message> { msg }, pCtx, add_assistant);
}

bool LLMInstance::InitializeChat(string system_prompt, std::vector<Message> messages)
{
	ModelState state = _atm_modelState.load();
	if (!state.bReady || !state.pModel)
		return false;

	_chatState = ChatState();
	_chatState.user.LoadFromXml("characters/user.xml"); // tmp
	_chatState.bot.LoadFromXml("characters/character.xml"); // tmp

	string prompt = trim(system_prompt);
	if (!isEmptyOrWhitespace(_chatState.bot.description))
	{
		prompt.append("\n");
		prompt.append(trim(_chatState.bot.description));
	}

	messages.insert(std::begin(messages), Message { Role::System, prompt });
	prompt = apply_chat_template(messages, state.pCtx, false);

	apply_names(prompt, _chatState.user.name, _chatState.bot.name);

	const llama_vocab* pVocab = llama_model_get_vocab(state.pModel);
	llama_context* pCtx = state.pCtx;

	// Tokenize system prompt
	_chatState.system_tokens = __tokenize(state.pModel, prompt, true);
	_chatState.current_pos = (int32_t)_chatState.system_tokens.size();
	_chatState.isInitialized = true;

	// Prepare assistant prelude
	string assistant_prefix = apply_chat_template(std::vector<Message>{}, state.pCtx, true);
	replace_all(assistant_prefix, "assistant", _chatState.bot.name);
	replace_all(assistant_prefix, "ASSISTANT", _chatState.bot.name);
	_chatState.assistant_tokens = __tokenize(state.pModel, assistant_prefix, false);

	// Pre-load system prompt into kv cache
	llama_kv_self_clear(pCtx);

	if (_chatState.batch.token != nullptr)
		llama_batch_free(_chatState.batch);

	// Prepare a batch for the prompt
	if (!__init_batch(state.pModel, state.pCtx, prompt, _chatState.batch))
		return false;

	if (_chatState.batch.n_tokens > 0 && llama_decode(state.pCtx, _chatState.batch))
	{
		fprintf(stderr, "failed to initialize chat\n");
		llama_batch_free(_chatState.batch);
		_chatState.batch = llama_batch {};
		return false;
	}

	return true;
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
	{
		printf(">> Loading. Waiting on worker thread ");
		fflush(stdout);
		_workerThread.get()->join();
		printf(">> Done!\r\n");
	}

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
	return state.bReady && state.pModel && _chatState.isInitialized;
}

bool LLMInstance::IsGenerating() const
{
	return _atm_bGeneratingResponse.load();
}

bool LLMInstance::Resume()
{
	if (!IsReady() || IsGenerating())
		return false;

	return Generate(Message { Role::User, "" }); //! @fix
}

bool LLMInstance::Halt()
{
	if (!IsReady() || !IsGenerating())
		return false;

	_atm_bCancelGeneration.store(true);
	if (_workerThread.get() != nullptr && _workerThread.get()->joinable())
	{
		printf(">> Halting. Waiting on worker thread ");
		fflush(stdout);
		_workerThread.get()->join();
		printf(">> Done!\r\n");
	}
	return true;
}

static void process(string& partial, string str_token, bool* bWait, bool* bHalt, string& stop_word)
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
		std::format("<{0}=\"", Constants::DialogueTagBegin),
		std::format("<{0}=\"", Constants::ActionTagBegin),
		std::format("<{0}=\"", Constants::ThoughtTagBegin),
		std::format("<{0}>", Constants::NarrationTagBegin),
	};

	static std::vector<std::string> closing_tags {
		std::format("<{0}>", Constants::DialogueTagEnd),
		std::format("<{0}>", Constants::ActionTagEnd),
		std::format("<{0}>", Constants::ThoughtTagEnd),
		std::format("<{0}>", Constants::NarrationTagEnd),
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
		stop_word = partial.substr(stop_pos);

		// Print to console
		partial = partial.erase(stop_pos); // Erase stop word
		*bHalt = true;
		*bWait = false;
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

static void refresh_block_positions(ChatState& chat)
{
	size_t pos = chat.system_tokens.size();
	for (auto& block : chat.blocks)
	{
		block.ctx_pos = (int32_t)pos;
		pos += block.tokens.size();
	}
}

std::vector<llama_token> __tokenize(llama_model* pModel, string prompt, bool bAddSpecial)
{
	const llama_vocab* pVocab = llama_model_get_vocab(pModel);

	std::vector<llama_token> prompt_tokens(1024);
	const int32_t n_prompt_tokens = llama_tokenize(pVocab, prompt.c_str(), (int32_t)prompt.size(), prompt_tokens.data(), (int32_t)prompt_tokens.size(), bAddSpecial, true);
	if (n_prompt_tokens < 0)
	{
		prompt_tokens.resize(-n_prompt_tokens);
		if (llama_tokenize(pVocab, prompt.c_str(), (int32_t)prompt.size(), prompt_tokens.data(), (int32_t)prompt_tokens.size(), bAddSpecial, true) < 0)
		{
			// Error
			return std::vector<llama_token> {};
		}
	}
	else
	{
		prompt_tokens.resize(n_prompt_tokens);
	}
	return prompt_tokens;
}

static bool __init_batch(llama_model* pModel, llama_context* pCtx, string prompt, llama_batch& out_pBatch)
{
	const int32_t maxCtx = llama_n_ctx(pCtx);
	const bool is_first = llama_kv_self_used_cells(pCtx) == 0;

	// tokenize the prompt
	std::vector<llama_token> prompt_tokens = __tokenize(pModel, prompt, is_first);

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

	out_pBatch = batch;
	return true;
}

void LLMInstance::__Generate(Message msg, ChatState* pChatState, __PartialResultCallback onPartial, __GenerationCompleteCallback onComplete)
{
	// Load state
	ModelState state = _atm_modelState.load();
	ChatState& chat = *pChatState;
	llama_batch& batch = chat.batch;
	const llama_vocab* pVocab = llama_model_get_vocab(state.pModel);

	string userName = chat.user.name;
	string botName = chat.bot.name;

	GenerationState genState;
	genState.messageId = ++_messageCounter;

	// Prepare prompt
	int32_t& current_pos = chat.current_pos;
	current_pos = llama_kv_self_used_cells(state.pCtx);
	int32_t start_pos = current_pos;
	int32_t n_batch  = llama_n_batch(state.pCtx);
	int32_t ctx_size  = llama_n_ctx(state.pCtx);

	std::vector<llama_token> prompt_tokens;

	// Insert user prompt
	string userPrompt = apply_chat_template(msg, state.pCtx, false);
	apply_names(userPrompt, userName, botName);
	auto userTokens = __tokenize(state.pModel, userPrompt, false);
	prompt_tokens.insert(std::begin(prompt_tokens), std::cbegin(userTokens), std::cend(userTokens));
	int32_t user_pos = start_pos;

	// Re-insert previous response (reformatted)
	for (auto it = std::rbegin(chat.blocks); it != std::rend(chat.blocks); ++it)
	{
		auto& lastBlock = *it;
		if (lastBlock.cached)
			break;

		string lastResponse = apply_chat_template(Message { lastBlock.role, lastBlock.content }, state.pCtx, false);
		auto lastTokens = __tokenize(state.pModel, lastResponse, false);

		lastBlock.content = lastResponse;
		lastBlock.tokens = lastTokens;
		//lastBlock.ctx_pos = start_pos;
		lastBlock.cached = true;

		prompt_tokens.insert(std::begin(prompt_tokens), std::cbegin(lastTokens), std::cend(lastTokens));
		user_pos = start_pos + (int32_t)lastTokens.size();
	}

	chat.blocks.push_back(LLMMessageBlock {
		/*role*/ Role::User,
		/*content*/ userPrompt,
		/*tokens*/ userTokens,
		/*ctx_pos*/ user_pos,
		/* cached */ true,
	});

	int32_t pos_pre_response = start_pos + (int32_t)prompt_tokens.size();

	if (pos_pre_response + Constants::MaxMessageTokens > ctx_size)
	{
		return;
	}
	refresh_block_positions(chat);

	// Append assistant tokens
	prompt_tokens.insert(std::end(prompt_tokens), std::begin(chat.assistant_tokens), std::end(chat.assistant_tokens));

	// Append to batch
	for (int i = 0; i < prompt_tokens.size(); ++i)
		common_batch_add(batch, prompt_tokens[i], current_pos + i, { 0 }, false);
	batch.logits[current_pos + prompt_tokens.size() - 1] = true;

	std::vector<llama_token> sampled_tokens;

	llama_token sampled_token;
	std::string partial;
	std::string stop_word;
	std::string response;
	MessageType msgType = MessageType::Undefined;

	printf("BEGIN GENERATION\r\n");

	while (true)
	{
		bool next_token = true;

		const int32_t n_tokens = std::min(n_batch, batch.n_tokens - current_pos);
		llama_batch batch_view = {
			n_tokens,
			batch.token + current_pos,
			nullptr,
			batch.pos + current_pos,
			batch.n_seq_id + current_pos,
			batch.seq_id + current_pos,
			batch.logits + current_pos,
		};

		// check if we have enough space in the context to evaluate this batch
		int n_ctx_used = llama_kv_self_used_cells(state.pCtx);
		if (n_ctx_used + n_tokens > ctx_size)
		{
			fprintf(stderr, "context size exceeded\n");
			onComplete(2, response);
			return;
		}
		if (batch_view.n_tokens > 0 && llama_decode(state.pCtx, batch_view))
		{
			fprintf(stderr, "failed to decode\n");
			onComplete(3, response);
			return;
		}

		current_pos += batch_view.n_tokens;

		// sample the next token
		sampled_token = llama_sampler_sample(state.pSampler, state.pCtx, -1);

		// is it an end of generation?
		if (llama_vocab_is_eog(pVocab, sampled_token))
		{
			partial.clear();
			break;
		}

		// convert the token to a string, print it and add it to the response
		std::string str_token = stringFromToken(pVocab, sampled_token);
		if (str_token.size() == 0)
			break; // Error

		partial += str_token;
		sampled_tokens.push_back(sampled_token);
		
		if (current_pos >= ctx_size)
			break; // Max limit reached

		bool send = true;

		// check if there is incomplete UTF-8 character at the end
		bool bHalt = false;
		bool bWait = false;
		process(partial, str_token, &bWait, &bHalt, stop_word);
		next_token &= !bHalt;
		send &= !bWait;

		if (_atm_bCancelGeneration.load())
			break; // Cancelled

		if (send)
		{
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

					if (tagName == userName) 
						break; // Stop if talking/acting for the user

					if (tag == Constants::DialogueTagEnd || tag == Constants::ActionTagEnd || tag == Constants::NarrationTagEnd || tag == Constants::ThoughtTagEnd)
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
							if (tag == Constants::DialogueTagBegin)
								msgType = MessageType::Dialogue;
							else if (tag == Constants::ActionTagBegin)
								msgType = MessageType::Action;
							else if (tag == Constants::NarrationTagBegin)
								msgType = MessageType::Narration;
							else if (tag == Constants::ThoughtTagBegin)
								msgType = MessageType::Thought;
						}
					}
				}
			}

			if (msgType == MessageType::Undefined)
				msgType = MessageType::Dialogue;

			// Send piece
			if (partial.size() > 0)
			{
				std::lock_guard<std::mutex> lock(_resultMutex);
				_generatedText += partial;

				_resultQueue.push(MessagePiece {
					genState.messageId,
					genState.currName,
					sendMsg,
					msgType,
					bEndOfMessageType,
				});
				response += partial;

				if (onPartial)
					onPartial(__PartialResult { partial, response });
			}

			partial = carryOver;
			if (bEndOfMessageType)
			{
				msgType = MessageType::Undefined;
				++genState.messageId;
			}

			send = false;
		}

		// Print to console
		printf("%s", str_token.c_str());

		common_batch_add(batch, sampled_token, current_pos, { 0 }, true);

		// prepare the next batch with the sampled token
		if (!next_token)
			break; // TODO: Carry over?
	}

	fflush(stdout);

	// Remove full response from cache (re-added, with formatting, next generation)
	llama_kv_self_seq_rm(state.pCtx, 0, pos_pre_response, -1);
	batch.n_tokens = pos_pre_response;
	chat.current_pos = pos_pre_response;

	chat.blocks.push_back(LLMMessageBlock {
		/*role*/ Role::Bot,
		/*content*/ response,
		/*tokens*/ sampled_tokens,
		/*ctx_pos*/ pos_pre_response,
		/*cached*/ false,
	});

	printf("\r\nEND OF GENERATION\r\n[%s](%s)\r\n", response.c_str(), stop_word.c_str());

	onComplete(0, response);
};

bool LLMInstance::Generate(Message msg)
{
	if (!IsReady() || IsGenerating())
		return false;

	if (_workerThread.get() != nullptr && _workerThread.get()->joinable())
	{
		_atm_bCancelGeneration.store(true);
		printf(">> Waiting on worker thread");
		fflush(stdout);
		_workerThread.get()->join();
		printf(">> Done!\r\n");
	}

	_atm_bCancelGeneration.store(false);
	_atm_bGeneratingResponse.store(true);

	auto pThread = new std::thread(&LLMInstance::__Generate, this, msg, &_chatState,
		[](__PartialResult partial) {
			// ...
		},
		[this](int error, string response) {
			// ...
			ModelState state = _atm_modelState.load();
			_chatState.prev_messages.clear();
			_chatState.prev_messages.push_back(Message { Role::Bot, response });

			_atm_bGeneratingResponse.store(false);
			ReportStatus();
		});

	_workerThread.reset(pThread);
	return true;
};

bool LLMInstance::SendMessage(Role role, string message, bool generate)
{
	if (!IsReady() || IsGenerating())
		return false;

	// Clear response queue
	{
		printf(">> Waiting on mutex ");
		fflush(stdout);
		std::lock_guard lock(_resultMutex);
		while (!_resultQueue.empty())
			_resultQueue.pop();
		printf(">> Done!\r\n");
	}
	_generatedText.clear();

	ModelState state = _atm_modelState.load();

	if (!generate) //! @fix
		return true;
	
	// generate a response
	return Generate( Message { role, message });
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
	std::unique_lock<std::mutex> lock(_resultMutex, std::try_to_lock);
	if(!lock.owns_lock())
		return false;

	if (_resultQueue.empty())
		return false;

	piece = _resultQueue.front();
	_resultQueue.pop();
	return true;
}