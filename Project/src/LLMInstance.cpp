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
	pGrammar = nullptr; // Freed by chain
	pCtx = nullptr;
	pModel = nullptr;
	bReady = false;
	bInvalid = false;
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
		DebugPrint(">> Releasing. Waiting on worker thread ");
		_workerThread.get()->join();
		DebugPrintLn(">> Done!");
	}

	Shutdown();
}

void LLMInstance::Shutdown()
{
	Halt();

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
	int n = llama_token_to_piece(pVocab, token, buf, sizeof(buf), 0, false);
	if (n < 0)
		return "";

	return std::string(buf, n);
}

size_t validate_utf8(const string& text)
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

	return string::npos;
}

size_t find_one_of(const string& text, const std::vector<string>& words)
{
	size_t stop_pos = string::npos;

	for (const string& word : words)
	{
		size_t pos = text.find(word);
		if (pos != string::npos && (stop_pos == string::npos || pos < stop_pos))
			stop_pos = pos;
	}

	return stop_pos;
}

size_t find_stopping_strings(const string& text, const std::vector<string>& stop_words, const size_t last_token_size, bool is_full_stop)
{
	size_t stop_pos = string::npos;

	for (const string& word : stop_words)
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

		if (pos != string::npos && (stop_pos == string::npos || pos < stop_pos))
		{
			stop_pos = pos;
		}
	}

	return stop_pos;
}

static void get_tag_and_name(const string& text, string& tag, string& name)
{
	size_t pos_equals = text.find('=', 1);
	if (pos_equals == string::npos)
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

static string apply_chat_template(Messages in_messages, llama_context* pCtx, bool add_assistant)
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
	return apply_chat_template(Messages { msg }, pCtx, add_assistant);
}

bool LLMInstance::InitializeChat(string system_prompt, Messages messages)
{
	ModelState state = _atm_modelState.load();
	if (!state.bReady || !state.pModel)
		return false;

	_chatState = ChatState();
	_chatState.user.LoadFromXml("characters/user.xml"); // tmp
	_chatState.bot.LoadFromXml("characters/character.xml"); // tmp

	// Initialize sampler + grammar
	const llama_vocab* pVocab = llama_model_get_vocab(state.pModel);

	// Init sampler chain
	if (state.pSampler == nullptr)
	{
		llama_sampler_chain_params sampler_params = llama_sampler_chain_default_params();
		llama_sampler* pSampler = llama_sampler_chain_init(sampler_params);

		// Load grammar
		string grammar = ReadTextFile("./resources/formatting_grammar.gbnf").value_or("");
		replace_all(grammar, "##NAME_PATTERN##", "(\"" + _chatState.bot.name + "\")");
		llama_sampler* grammar_sampler = llama_sampler_init_grammar(pVocab, grammar.c_str(), "root");
		if (grammar_sampler)
			DebugPrintLn("Grammar loaded");

		if (grammar_sampler) llama_sampler_chain_add(pSampler, grammar_sampler);					// Grammar
		llama_sampler_chain_add(pSampler, llama_sampler_init_min_p(0.15f, 1));						// Min P sampler
		llama_sampler_chain_add(pSampler, llama_sampler_init_temp(1.5f));							// Temperature
		llama_sampler_chain_add(pSampler, llama_sampler_init_penalties(512, 1.05f, 0.0f, 0.0f));	// Repeat penalty
		llama_sampler_chain_add(pSampler, llama_sampler_init_dist(LLAMA_DEFAULT_SEED));				// Seed

		state.pSampler = pSampler;
		state.pGrammar = grammar_sampler;
		_atm_modelState.store(state);
	}

	// Init system prompt
	string prompt = trim(system_prompt);
	if (!isEmptyOrWhitespace(_chatState.bot.description))
	{
		string persona;
		persona.reserve(_chatState.bot.description.size() + 20);
		prompt.append("# About {{char}}:\n");
		prompt.append(trim(_chatState.bot.description));
		replace(prompt, "##CHARACTER_INFO##", persona);
	}

	messages.insert(std::begin(messages), Message { Role::System, prompt });
	prompt = apply_chat_template(messages, state.pCtx, false);

	apply_names(prompt, _chatState.user.name, _chatState.bot.name);

	llama_context* pCtx = state.pCtx;

	// Tokenize system prompt
	_chatState.system_tokens = __tokenize(state.pModel, prompt, true);
	_chatState.current_pos = (int32_t)_chatState.system_tokens.size();
	_chatState.isInitialized = true;

	// Prepare assistant prelude
	string assistant_prefix = apply_chat_template(Messages{}, state.pCtx, true);
	replace(assistant_prefix, "assistant", _chatState.bot.name);
	replace(assistant_prefix, "ASSISTANT", _chatState.bot.name);
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

bool LLMInstance::Restart()
{
	ModelState state = _atm_modelState.load();
	if (!IsReady())
		return false;

	llama_context* pCtx = state.pCtx;
	const int32_t maxCtx = llama_n_ctx(pCtx);

	auto& prompt_tokens = _chatState.system_tokens;

	// Reset batch pointer
	_chatState.current_pos = (int32_t)prompt_tokens.size();
	_chatState.blocks.clear();
	_chatState.isInitialized = true;

	llama_kv_self_clear(pCtx);

	// Reinit the batch
	int32_t num_tokens = (int32_t)prompt_tokens.size();

	// Add tokens to batch
	auto& batch = _chatState.batch;
	for (int i = 0; i < num_tokens; ++i) {
		batch.token[i] = prompt_tokens[i];
		batch.pos[i] = i;  // Position in sequence
		batch.n_seq_id[i] = 1;  // This token belongs to 1 sequence
		batch.seq_id[i][0] = 0;  // Sequence ID 0
		batch.logits[i] = false;  // Don't need logits for most tokens
	}
	batch.logits[num_tokens - 1] = true;  // Only need logits for last token
	batch.n_tokens = num_tokens;

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

	state.bReady = true;
	state.bInvalid = false;

	onComplete(state);
}

bool LLMInstance::LoadModelAsync(string filename, LoadModelProgressCallback onProgress, LoadModelCallback onComplete)
{
	if (IsReady())
		return false; // Already loaded

	if (_workerThread.get() != nullptr && _workerThread.get()->joinable())
	{
		DebugPrint(">> Loading. Waiting on worker thread ");
		_workerThread.get()->join();
		DebugPrintLn(">> Done!");
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
		}
		else
		{
			result.Release();
			_modelName.clear();
			onComplete(false);
		}
	});

	_workerThread.reset(pThread);

	return true;
}

bool LLMInstance::IsReady() const
{
	ModelState state = _atm_modelState.load();
	return state.bReady && !state.bInvalid && state.pModel && _chatState.isInitialized;
}

bool LLMInstance::IsGenerating() const
{
	return _atm_bGeneratingResponse.load();
}

bool LLMInstance::Resume()
{
	if (!IsReady() || IsGenerating())
		return false;
	return false; // Todo
}

bool LLMInstance::Halt()
{
	if (!IsReady() || !IsGenerating())
		return false;

	CancelWorkerThread();
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

	static std::vector<string> stop_words {
		"<|",
		"<end_of_turn",
		"<EOT>",
		"_<EOT>",
		"<s>",
		"</s>",
		"### ",
		"<｜",
		"\r\r",
//		"<|end",
//		"<｜end▁of▁sentence｜>",
	};

	static std::vector<string> opening_tags {
		std::format("<{0}=\"", Constants::DialogueTagBegin),
		std::format("<{0}=\"", Constants::ActionTagBegin),
		std::format("<{0}=\"", Constants::ThoughtTagBegin),
		std::format("<{0}>", Constants::NarrationTagBegin),
	};

	static std::vector<string> closing_tags {
		std::format("<{0}>", Constants::DialogueTagEnd),
		std::format("<{0}>", Constants::ActionTagEnd),
		std::format("<{0}>", Constants::ThoughtTagEnd),
		std::format("<{0}>", Constants::NarrationTagEnd),
	};

	static std::vector<string> formatting_tags;
	if (formatting_tags.empty())
	{
		formatting_tags.insert(std::end(formatting_tags), std::begin(opening_tags), std::end(opening_tags));
		formatting_tags.insert(std::end(formatting_tags), std::begin(closing_tags), std::end(closing_tags));
	}

	// Look for stop word - and halt
	size_t stop_pos = find_stopping_strings(partial, stop_words, str_token.size(), true);
	if (stop_pos != string::npos)
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
	if (stop_pos != string::npos)
	{
		*bHalt = false;
		*bWait = true;
		return;
	}

	// Look for formatting tags
	size_t fmt_pos = find_one_of(partial, opening_tags);
	if (fmt_pos != string::npos)
	{
		// Await end of tag '>', or beginning of a new tag '<' (indicating garbage from the model)
		if (partial.find_first_of("<>", fmt_pos + 1, 2) == string::npos)
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
		if (fmt_pos != string::npos)
		{
			*bHalt = false;
			*bWait = true;
			return;
		}
	}

	*bHalt = false;
	*bWait = false;
}

static void assign_block_positions(ChatState& chat)
{
	size_t pos = chat.system_tokens.size();
	for (auto& block : chat.blocks)
	{
		block.ctx_pos = (int32_t)pos;
		pos += block.tokens.size();
	}
}

static void init_batch_logits(llama_batch& batch)
{
	if (batch.n_tokens <= 0)
		return;

	for (int i = 0; i < batch.n_tokens - 1; ++i)
		batch.logits[i] = false;
	batch.logits[batch.n_tokens - 1] = true;  // Only need logits for last token
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

void LLMInstance::PrepareGeneration(PrepareArguments args)
{
	// Load state
	ModelState state = _atm_modelState.load();
	ChatState& chat = *args.pChatState;
	llama_batch& batch = chat.batch;
	const llama_vocab* pVocab = llama_model_get_vocab(state.pModel);

	string userName = chat.user.name;
	string botName = chat.bot.name;

	// Prepare prompt
	int32_t& current_pos = chat.current_pos;
	current_pos = llama_kv_self_used_cells(state.pCtx);
	int32_t start_pos = current_pos;
	int32_t n_batch = llama_n_batch(state.pCtx);
	int32_t ctx_size  = llama_n_ctx(state.pCtx);

	std::vector<llama_token> prompt_tokens;

	// Tokenize uncached messages
	for (auto it = std::begin(chat.blocks); it != std::end(chat.blocks); ++it)
	{
		auto& block = *it;
		if (block.cached)
			continue;

		string content = apply_chat_template(Message { block.role, block.content }, state.pCtx, false);
		apply_names(content, userName, botName);

		auto lastTokens = __tokenize(state.pModel, content, false);

		block.content = content;
		block.tokens = lastTokens;
		block.ctx_pos = 0; // assigned later
		block.cached = true;

		prompt_tokens.insert(std::end(prompt_tokens), std::cbegin(lastTokens), std::cend(lastTokens));
	}

	// Shift context window
	if (current_pos + (int32_t)prompt_tokens.size() + Constants::MaxMessageTokens > ctx_size)
	{
		size_t keep_tokens = static_cast<int32_t>(ctx_size * Constants::ContextWindowSizeRatio) - (int32_t)prompt_tokens.size();
		
		size_t total = 0;
		int32_t first_to_keep = (int32_t)chat.blocks.size() - 1;
		for (; first_to_keep >= 0; --first_to_keep)
		{
			if (total + chat.blocks[first_to_keep].tokens.size() >= keep_tokens)
				break;
			total += (int32_t)chat.blocks[first_to_keep].tokens.size();
		}
		size_t shift_amount = 0;
		for (int32_t i = 0; i < first_to_keep; ++i)
			shift_amount += chat.blocks[i].tokens.size();
		chat.blocks.erase(std::cbegin(chat.blocks), std::begin(chat.blocks) + (ptrdiff_t)first_to_keep);

		int32_t pos_remove_begin = (int32_t)chat.system_tokens.size();
		int32_t pos_remove_end = pos_remove_begin + (int32_t)shift_amount;

		bool removed = llama_kv_self_seq_rm(state.pCtx, 0, pos_remove_begin, pos_remove_end);
		llama_kv_self_seq_add(state.pCtx, 0, (llama_pos)pos_remove_end, -1, -(llama_pos)shift_amount); // shift up

		start_pos -= (int32_t)shift_amount;
		current_pos -= (int32_t)shift_amount;

		// Shift batch
		for (int32_t i = 0; i < n_batch - pos_remove_end; ++i)
		{
			batch.token[pos_remove_begin + i] = batch.token[pos_remove_end + i];
			batch.logits[i] = false;  // Don't need logits for most tokens
		}
		batch.n_tokens = batch.n_tokens - (int32_t)shift_amount;
	}

	assign_block_positions(chat);

	chat.pre_response_pos = start_pos + (int32_t)prompt_tokens.size();

	// Append assistant tokens
	if (args.responder == Responder::Bot)
		prompt_tokens.insert(std::end(prompt_tokens), std::begin(chat.assistant_tokens), std::end(chat.assistant_tokens));
	else if (args.responder != Responder::None)
	{
		string responderName;
		if (args.responder == Responder::Narrator)
			responderName = "Narrator";
		else if (args.responder == Responder::User)
			responderName = chat.user.name;
		else
			responderName = chat.bot.name; // Fallback

		// Prepare assistant prelude
		string assistant_prefix = apply_chat_template(Messages{}, state.pCtx, true);
		replace(assistant_prefix, "assistant", responderName);
		replace(assistant_prefix, "ASSISTANT", responderName);
		auto assistant_tokens = __tokenize(state.pModel, assistant_prefix, false);
		prompt_tokens.insert(std::end(prompt_tokens), std::begin(assistant_tokens), std::end(assistant_tokens));
	}

	// Append to batch
	for (int i = 0; i < prompt_tokens.size(); ++i)
		common_batch_add(batch, prompt_tokens[i], current_pos + i, { 0 }, false);
	batch.logits[current_pos + prompt_tokens.size() - 1] = true;

	chat.prepend_pos = current_pos + (int32_t)prompt_tokens.size();
}

void LLMInstance::Generate(GenerateArguments args, __PartialResultCallback onPartial, __GenerationCompleteCallback onComplete)
{
	std::vector<llama_token> sampled_tokens;
	ModelState state = _atm_modelState.load();
	const llama_vocab* pVocab = llama_model_get_vocab(state.pModel);

	llama_token sampled_token;
	string partial;
	string stop_word;
	string response;
	MessageType msgType = args.msgType;

	ChatState& chat = *args.pChat;
	llama_batch& batch = chat.batch;
	int32_t n_batch = llama_n_batch(state.pCtx);
	int32_t ctx_size  = llama_n_ctx(state.pCtx);
	int32_t& current_pos = chat.current_pos;
	string userName = chat.user.name;
	string botName = chat.bot.name;
	int32_t pre_response_pos = chat.pre_response_pos;

	GenerationState genState;
	genState.messageId = ++_messageCounter;

	DebugPrintLn(">> BEGIN GENERATION");

	if (state.pGrammar)
		llama_sampler_reset(state.pGrammar);

	if (!args.prepend.empty())
	{
		auto prepend_tokens = __tokenize(state.pModel, args.prepend, false);

		// Append to batch
		for (int i = 0; i < prepend_tokens.size(); ++i)
			common_batch_add(batch, prepend_tokens[i], chat.prepend_pos + i, { 0 }, false);
		batch.logits[chat.prepend_pos + prepend_tokens.size() - 1] = true;

		partial += args.prepend;
		printf("%s", partial.c_str());
	}
	init_batch_logits(batch);

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
			onComplete(InternalError::ContextFull, "context size exceeded");
			return;
		}
		
		if (batch_view.n_tokens > 0 && llama_decode(state.pCtx, batch_view))
		{
			onComplete(InternalError::DecodeError, "llama_decode returned error");
			return;
		}

		current_pos += batch_view.n_tokens;

		// sample the next token
		try
		{
			sampled_token = llama_sampler_sample(state.pSampler, state.pCtx, -1);
		}
		catch (const std::runtime_error& e)
		{
			if (strstr(e.what(), "Unexpected empty grammar stack") != 0)
				onComplete(InternalError::GrammarError, e.what());
			else
				onComplete(InternalError::SamplerError, e.what());
			return;
		}

		// is it an end of generation?
		if (llama_vocab_is_eog(pVocab, sampled_token))
		{
			partial.clear();
			break;
		}

		// convert the token to a string, print it and add it to the response
		string str_token = stringFromToken(pVocab, sampled_token);
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
			if (fmt_start != string::npos)
			{
				bool bRemove = false;
				size_t fmt_end = partial.find('>', fmt_start + 1);
				if (fmt_end != string::npos)
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
				if (args.maxMessages > 0)
					break; // That's enough, thank you
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
	llama_kv_self_seq_rm(state.pCtx, 0, pre_response_pos, -1);
	batch.n_tokens = pre_response_pos;
	chat.current_pos = pre_response_pos;

	chat.blocks.push_back(LLMMessageBlock {
		/*role*/ Role::Bot,
		/*content*/ response,
		/*tokens*/ sampled_tokens,
		/*ctx_pos*/ pre_response_pos,
	});

	DebugPrintLn();
	DebugPrintLn();
	DebugPrintLn(std::format("END OF GENERATION (stopped on:{})", stop_word.c_str()));

	onComplete(InternalError::NoError, response);
};

void LLMInstance::CancelWorkerThread()
{
	_atm_bCancelGeneration.store(true);

	if (_workerThread.get() != nullptr && _workerThread.get()->joinable())
	{
		DebugPrint(">> Halting. Waiting on worker thread ");
		_workerThread.get()->join();
		DebugPrintLn(">> Done!");
	}
	_atm_bGeneratingResponse.store(false);
}

void LLMInstance::ClearResponseQueue()
{
	DebugPrintLn(">> Waiting on mutex ");
	std::lock_guard lock(_resultMutex);
	while (!_resultQueue.empty())
		_resultQueue.pop();
	DebugPrintLn(">> Done!");
}

bool LLMInstance::SendMessage(Role role, string message)
{
	if (!IsReady() || IsGenerating())
		return false;

	if (_workerThread.get() != nullptr && _workerThread.get()->joinable())
	{
		_atm_bCancelGeneration.store(true);
		DebugPrint(">> Waiting on worker thread ");
		_workerThread.get()->join();
		DebugPrintLn(">> Done!");
	}
	_atm_bCancelGeneration.store(false);

	PushMessage(role, message);

	PrepareArguments prepareArgs {
		/*chat state*/ &_chatState,
		/*responder */ Responder::Bot,
	};
	PrepareGeneration(prepareArgs);

	_atm_bGeneratingResponse.store(true);
	GenerateArguments generateArgs {
		/*chat state*/ &_chatState,
	};

	auto pThread = new std::thread(&LLMInstance::Generate, this, generateArgs,
		[](__PartialResult partial) {
			// ...
		},
		[this](InternalError error, string response) {
			// ...
			ModelState state = _atm_modelState.load();
			if (error != InternalError::NoError)
			{
				DebugPrintLn();
				DebugPrintLn(std::format(">> Internal error: ({}) {}", (int)error, response.c_str()));
				state.bInvalid = true; // Invalidate state
				_atm_modelState.store(state);
			}
			_atm_bGeneratingResponse.store(false);
		});

	_workerThread.reset(pThread);
	return true;
}

bool LLMInstance::PushMessage(Role role, string message)
{
	if (!IsReady() || IsGenerating())
		return false;

	_chatState.blocks.push_back(LLMMessageBlock {
		/*role*/ role,
		/*content*/ message,
		/*tokens*/ {},
		/*ctx_pos*/ 0,
		/* cached */ false,
	});
	return true;
}

int LLMInstance::RemoveMessages(int numMessages)
{
	if (!IsReady() || IsGenerating() || numMessages == 0)
		return 0;

	int32_t newSize = std::max((int32_t)_chatState.blocks.size() - numMessages, 0);
	int32_t& current_pos = _chatState.current_pos;

	if (newSize > 0)
	{
		auto& block = _chatState.blocks[newSize - 1];
		if (block.cached)
			current_pos = std::min(current_pos, block.ctx_pos + (int32_t)block.tokens.size());
		else
			current_pos = std::min(current_pos, block.ctx_pos);
	}
	else
	{
		current_pos = (int32_t)_chatState.system_tokens.size();
	}
	
	// Update batch
	_chatState.batch.n_tokens = current_pos;

	// Clear kv cache
	ModelState state = _atm_modelState.load();
	llama_kv_self_seq_rm(state.pCtx, 0, current_pos, -1);

	int removed = (int32_t)_chatState.blocks.size() - newSize;
	_chatState.blocks.resize((size_t)newSize);
	return removed;
}

bool LLMInstance::Instigate(Responder responder, MessageType msgType, int messageCount)
{
	if (!IsReady() || IsGenerating() || responder == Responder::None)
		return false;

	if (_workerThread.get() != nullptr && _workerThread.get()->joinable())
	{
		_atm_bCancelGeneration.store(true);
		DebugPrint(">> Waiting on worker thread ");
		_workerThread.get()->join();
		DebugPrintLn(">> Done!");
	}

	_atm_bCancelGeneration.store(false);
	_atm_bGeneratingResponse.store(true);

	PrepareArguments prepareArgs {
		/*chat state*/ &_chatState,
		/*responder */ responder,
	};
	PrepareGeneration(prepareArgs);

	string responderName = responder == Responder::User ? _chatState.user.name : _chatState.bot.name;

	string prependMsg;
	if (msgType == MessageType::Dialogue)
		prependMsg = std::format("<{}=\"{}\">", Constants::DialogueTagBegin, responderName);
	if (msgType == MessageType::Action)
		prependMsg = std::format("<{}=\"{}\">", Constants::ActionTagBegin, responderName);
	if (msgType == MessageType::Thought)
		prependMsg = std::format("<{}=\"{}\">", Constants::ThoughtTagBegin, responderName);
	else if (msgType == MessageType::Narration)
		prependMsg = std::format("<{}>", Constants::NarrationTagBegin);

	GenerateArguments generateArgs {
		/*chat state*/ &_chatState,
		/*msgType*/ msgType,
		/*maxMessageCount*/ messageCount,
		/*prepend*/ prependMsg,
	};

	auto pThread = new std::thread(&LLMInstance::Generate, this, generateArgs,
		[](__PartialResult partial) {
			// ...
		},
		[this](InternalError error, string response) {
			// ...
			ModelState state = _atm_modelState.load();
			if (error != InternalError::NoError)
			{
				printf("\r\n>> Internal error: (%d) %s\r\n", error, response.c_str());
				state.bInvalid = true; // Invalidate state
				_atm_modelState.store(state);
			}
			_atm_bGeneratingResponse.store(false);
		});

	_workerThread.reset(pThread);
	return true;
}


LLMStatus LLMInstance::GetStatus() const
{
	ModelState state = _atm_modelState.load();

	LLMStatus status {};
	if (state.pModel && state.pCtx)
	{
		status.modelName = _modelName;
		status.allocCtxSize = llama_n_ctx(state.pCtx);
		status.usedCtxSize = llama_kv_self_used_cells(state.pCtx);
		status.bReady = state.bReady;
		status.bInvalid = state.bInvalid;
	}
	return status;
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

bool LLMInstance::DumpContext(string filename) const
{
	if (!IsReady())
		return false;

	ModelState state = _atm_modelState.load();
	const llama_vocab* pVocab = llama_model_get_vocab(state.pModel);

	auto fnTokenStr = [pVocab](llama_token token) -> string {
		if (token == llama_vocab_bos(pVocab))
			return "<BOS>";
		else if (token == llama_vocab_eos(pVocab))
			return "<EOS>";
		else if (token == llama_vocab_eot(pVocab))
			return "<EOT>";
		else if (token == llama_vocab_sep(pVocab))
			return "<SEP>";
		else if (token == llama_vocab_pad(pVocab))
			return "<PAD>";
		else if (token == llama_vocab_nl(pVocab))
			return "\r\n";
		else
		{
			char buf[256];
			int n = llama_token_to_piece(pVocab, token, buf, sizeof(buf), 0, true);
			if (n < 0)
				return "<UNK>";
			else
				return string(buf, n);
		}
	};

	const llama_batch& batch = _chatState.batch;

	// Detokenize the batched tokens
	string result;
	result.reserve(batch.n_tokens * 4); // Rough estimate for string size

	for (int32_t i = 0; i < batch.n_tokens; ++i)
		result.append(fnTokenStr(batch.token[i]));

	for (auto& block : _chatState.blocks)
	{
		if (block.cached)
			continue;

		result.append("[");
		for (int32_t i = 0; i < block.tokens.size(); ++i)
		{
			result.append(fnTokenStr(block.tokens[i]));
		}
		result.append("]\r\n");
	}

	return WriteTextFile(filename, result, false);
}