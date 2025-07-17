#include "LLMInstance.h"
#include "common.h"
#include "StringUtil.h"
#include "Utility.h"
#include "Message.h"
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

	_atm_bGeneratingResponse.store(false);
	_atm_modelState.store(ModelState());
}

LLMInstance::~LLMInstance()
{
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

static const char* name_from_role(Role role)
{
	static const char* SYSTEM_NAME = "system";
	static const char* NARRATOR_NAME = "Narrator";
	static const char* DIRECTOR_NAME = "Director";
	static const char* USER_NAME = "{{user}}";
	static const char* BOT_NAME = "{{char}}";

	switch (role)
	{
	case Role::Bot: return BOT_NAME;
	case Role::User: return USER_NAME;
	case Role::System: return SYSTEM_NAME;
	case Role::Narrator: return NARRATOR_NAME;
	case Role::Director: return DIRECTOR_NAME;
	}
	return "";
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


	std::vector<llama_chat_message> llama_msgs(in_messages.size());
	for (int i = 0; i < in_messages.size(); ++i)
	{
		auto& msg = in_messages[i];
		llama_msgs[i] = llama_chat_message { 
			msg.name.empty() ? name_from_role(msg.role) : msg.name.c_str(), 
			msg.content.c_str() 
		};
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
		std::format("<{0}=\"", Constants::DialogueTag),
		std::format("<{0}=\"", Constants::ActionTag),
		std::format("<{0}=\"", Constants::ThoughtTag),
		std::format("<{0}>", Constants::NarrationTag),
		std::format("<{0}>", Constants::DirectionTag),
	};

	static std::vector<string> closing_tags {
		std::format("</{0}>", Constants::DialogueTag),
		std::format("</{0}>", Constants::ActionTag),
		std::format("</{0}>", Constants::ThoughtTag),
		std::format("</{0}>", Constants::NarrationTag),
		std::format("</{0}>", Constants::DirectionTag),
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
		pos += block.length();
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

static std::vector<llama_token> __tokenize(llama_model* pModel, string prompt, bool bAddSpecial)
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

static int32_t remove_and_shift(llama_context* pCtx, ChatState& chat, std::vector<LLMMessageBlock>::iterator itBegin, std::vector<LLMMessageBlock>::iterator itEnd)
{
	// Remove
	llama_pos shift_amount = 0;
	for (auto it = itBegin; it != itEnd; ++it)
		shift_amount += (llama_pos)(*it).length();
	
	llama_pos pos_remove_begin = (*itBegin).ctx_pos;
	llama_pos pos_remove_end = pos_remove_begin + shift_amount;

	if (llama_kv_self_seq_rm(pCtx, 0, pos_remove_begin, pos_remove_end))
		chat.blocks.erase(itBegin, itEnd);
	else
		return 0; // Error

	// Shift
	llama_kv_self_seq_add(pCtx, 0, pos_remove_end, -1, -shift_amount);

	auto& batch = chat.batch;
//	int32_t n_batch = (int32_t)llama_n_batch(pCtx);
	int32_t n_batch = batch.n_tokens;
	for (int32_t i = 0; i < n_batch - pos_remove_end; ++i)
	{
		batch.token[pos_remove_begin + i] = batch.token[pos_remove_end + i];
		batch.logits[i] = false;
	}
	batch.n_tokens -= shift_amount;
	return (int32_t)shift_amount;
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
#if _DEBUG
		llama_sampler_chain_add(pSampler, llama_sampler_init_dist(0xA0B0C0D0));				// Seed
#else
		llama_sampler_chain_add(pSampler, llama_sampler_init_dist(LLAMA_DEFAULT_SEED));				// Seed
#endif

		state.pSampler = pSampler;
		state.pGrammar = grammar_sampler;
		_atm_modelState.store(state);
	}

	// Init system prompt
	string prompt = trim(system_prompt);
	if (!empty_or_whitespace(_chatState.bot.description))
	{
		string persona;
		persona.reserve(_chatState.bot.description.size() + 20);
		persona.append("# About {{char}}:\n");
		persona.append(trim(_chatState.bot.description));
		replace(prompt, "##CHARACTER_INFO##", persona);
	}	
	if (!empty_or_whitespace(_chatState.user.description))
	{
		string user_persona;
		user_persona.reserve(_chatState.user.description.size() + 20);
		user_persona.append("# About {{user}}:\n");
		user_persona.append(trim(_chatState.user.description));
		replace(prompt, "##USER_INFO##", user_persona);
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

bool LLMInstance::ResetChat(int seed)
{
	ModelState state = _atm_modelState.load();
	if (!CanGenerate())
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

	if (seed > 0)
		Reseed(seed);
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
	if (IsReady() || _bLoadingModel)
		return false; // Already loaded

	CancelGeneration();

	_bLoadingModel = true;

	__LoadModelProgressCallback = onProgress;

	_workerThread = std::make_unique<std::jthread>(std::jthread(__LoadModel,
		filename,
		[this, filename, onComplete](ModelState result)
	{
		_bLoadingModel = false;
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
	}));

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

bool LLMInstance::CanGenerate() const
{
	return IsReady() && !IsGenerating();
}

bool LLMInstance::Resume()
{
	if (!CanGenerate())
		return false;
	return false; // Todo
}

bool LLMInstance::Halt()
{
	if (!IsReady() || !IsGenerating())
		return false;

	CancelGeneration();
	_atm_bGeneratingResponse.store(false);
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

		prompt_tokens.insert(std::end(prompt_tokens), std::cbegin(lastTokens), std::cend(lastTokens));
	}

	// Shift context window
	size_t ctx_reserve = prompt_tokens.size() + Constants::MaxResponseLength;
	if (current_pos + ctx_reserve > ctx_size)
	{
		size_t ctx_chat_max = ctx_size - chat.system_tokens.size(); // Exclude system prompt
		size_t free_tokens = std::max(static_cast<int32_t>(ctx_reserve), static_cast<int32_t>(ctx_chat_max * (1.0f - Constants::ContextWindowKeepRatio)));
		
		size_t total = 0;
		size_t first_to_keep = 0;
		while (first_to_keep < chat.blocks.size() && total < free_tokens && chat.blocks[first_to_keep].cached)
			total += (int32_t)chat.blocks[first_to_keep++].length();

		current_pos -= remove_and_shift(state.pCtx, chat, std::begin(chat.blocks), std::begin(chat.blocks) + (ptrdiff_t)first_to_keep);
	}

	// Calculate block positions
	assign_block_positions(chat);

	// Store response position
	chat.pre_response_pos = current_pos + (int32_t)prompt_tokens.size();

	// Append assistant tokens
	if (args.responder == Responder::Bot)
		prompt_tokens.insert(std::end(prompt_tokens), std::begin(chat.assistant_tokens), std::end(chat.assistant_tokens));
	else if (args.responder != Responder::None)
	{
		string responderName;
		if (args.responder == Responder::Narrator)
			responderName = name_from_role(Role::Narrator);
		else if (args.responder == Responder::User)
			responderName = chat.user.name;
		else
			responderName = chat.bot.name; // Fallback

		// Prepare assistant prelude
		string assistant_prelude = apply_chat_template(Messages{}, state.pCtx, true);
		replace(assistant_prelude, "assistant", responderName);
		replace(assistant_prelude, "ASSISTANT", responderName);
		auto assistant_tokens = __tokenize(state.pModel, assistant_prelude, false);
		prompt_tokens.insert(std::end(prompt_tokens), std::begin(assistant_tokens), std::end(assistant_tokens));
	}

	// Store beginning of response (after assistant prelude)
	chat.prepend_pos = current_pos + (int32_t)prompt_tokens.size();

	// Append to batch
	for (int i = 0; i < prompt_tokens.size(); ++i)
		common_batch_add(batch, prompt_tokens[i], current_pos + i, { 0 }, false);
	batch.logits[current_pos + prompt_tokens.size() - 1] = true;

	// Mark blocks in cache (they will be shortly)
	for (auto it = std::begin(chat.blocks); it != std::end(chat.blocks); ++it)
		it->cached = true;

}

void LLMInstance::Generate(std::stop_token thread_stop, GenerateArguments args, __PartialResultCallback onPartial, __GenerationCompleteCallback onComplete)
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

	uuid responseId = CreateUUID();
	uuid subMessageId = CreateUUID();
	int numMessages = 0;
	string responderName {};

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

		if (thread_stop.stop_requested())
			break;

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

					if (tagName == userName && args.role != Role::User)
						break; // Stop if talking/acting for the user

					if (tag.size() > 1 && tag[0] == '/')
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
							responderName = tagName;
							if (tag == Constants::DialogueTag)
								msgType = MessageType::Dialogue;
							else if (tag == Constants::ActionTag)
								msgType = MessageType::Action;
							else if (tag == Constants::ThoughtTag)
								msgType = MessageType::Thought;
							else if (tag == Constants::NarrationTag)
								msgType = MessageType::Narration;
							else if (tag == Constants::DirectionTag)
								msgType = MessageType::Direction;
						}
					}
				}
			}

			if (msgType == MessageType::Undefined)
				msgType = MessageType::Dialogue;

			// Send piece
			if (partial.size() > 0)
			{
				std::scoped_lock lock(_resultMutex);

				_resultQueue.push(MessagePiece {
					/*responseId*/ responseId,
					/*subMessageId*/ subMessageId,
					/*name*/ responderName,
					/*text*/ sendMsg,
					/*msgType*/ args.role == Role::User ? MessageType::UserMessage : msgType,
					/*isComplete*/bEndOfMessageType,
				});
				response += partial;

				if (onPartial)
					onPartial(__PartialResult { partial, response });
			}

			partial = carryOver;
			if (bEndOfMessageType)
			{
				msgType = MessageType::Undefined;
				if (args.maxMessages > 0 && ++numMessages >= args.maxMessages)
					break; // That's enough, thank you
				subMessageId = CreateUUID();
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
		/*responseId*/ responseId,
		/*role*/ args.role,
		/*content*/ response,
		/*tokens*/ sampled_tokens,
		/*ctx_pos*/ pre_response_pos,
	});

	DebugPrintLn();
	DebugPrintLn();
	DebugPrintLn(std::format("END OF GENERATION (stopped on:{})", stop_word.c_str()));

	onComplete(InternalError::NoError, response);
};

void LLMInstance::CancelGeneration()
{
	if (_workerThread.get() && _workerThread->joinable())
	{
		DebugPrint(">> Stoping worker thread ");
		_workerThread->request_stop();
		_workerThread->join();
		DebugPrintLn(">> Done!");
	}
}

void LLMInstance::ClearResponseQueue()
{
	DebugPrintLn(">> Waiting on mutex ");
	std::scoped_lock lock(_resultMutex);
	while (!_resultQueue.empty())
		_resultQueue.pop();
	DebugPrintLn(">> Done!");
}

bool LLMInstance::SendMessage(Role role, string message)
{
	if (!CanGenerate())
		return false;

	if (empty_or_whitespace(message))
		return false;

	CancelGeneration();

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

	_workerThread = std::make_unique<std::jthread>(std::jthread(std::bind_front(&LLMInstance::Generate, this), generateArgs,
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
		}));

	return true;
}

bool LLMInstance::PushMessage(Role role, string message, MessageType msgType, bool visible)
{
	if (!CanGenerate())
		return false;

	if (empty_or_whitespace(message))
		return false;

	// Process
	string name = name_from_role(role);
	string formatted = FormatMessage(message, name);

	uuid blockId = CreateUUID();
	uuid messageId = CreateUUID();

	_chatState.blocks.push_back(LLMMessageBlock {
		/*blockId*/ blockId,
		/*role*/ role,
		/*content*/ formatted,
		/*tokens*/ {},
		/*ctx_pos*/ 0,
		/*cached*/ false,
	});

	if (visible)
	{
		if (role == Role::Bot)
			name = _chatState.bot.name;
		else if (role == Role::User)
			name = _chatState.user.name;

		std::scoped_lock lock(_resultMutex);
		
		// Add message to result queue
		_resultQueue.push(MessagePiece {
			/*blockId*/ blockId,
			/*messageId*/ messageId,
			/*name*/ name,
			/*text*/ message,
			/*msgType*/ msgType,
			/*isComplete*/ true,
			});
	}
	return true;
}

std::vector<uuid> LLMInstance::RemoveMessages(int numMessages)
{
	if (!CanGenerate() || numMessages < 1)
		return {};

	int32_t newSize = std::max((int32_t)_chatState.blocks.size() - numMessages, 0);
	int32_t& current_pos = _chatState.current_pos;

	if (newSize > 0)
	{
		auto& block = _chatState.blocks[newSize - 1];
		if (block.cached)
			current_pos = std::min(current_pos, block.ctx_pos + (int32_t)block.length());
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

	// Return removed ids
	std::vector<uuid> removedIds;
	removedIds.reserve((int32_t)_chatState.blocks.size() - newSize);
	for (size_t i = (size_t)newSize; i < _chatState.blocks.size(); ++i)
		removedIds.push_back(_chatState.blocks[i].responseId);
	
	// Remove blocks
	_chatState.blocks.resize((size_t)newSize);
	return removedIds;
}

std::vector<uuid> LLMInstance::RollbackUserMessage()
{
	if (!CanGenerate())
		return {};

	for (int i = (int32_t)_chatState.blocks.size() - 1; i >= 0; --i)
	{
		if (_chatState.blocks[i].role == Role::User)
			return RemoveMessages((int32_t)_chatState.blocks.size() - i);
	}
	return {};
}

bool LLMInstance::GreetUser()
{
	if (!CanGenerate())
		return false;

	string prompt = ReadTextFile("./resources/prompt_greeting.txt").value_or("{{char}} greets {{user}}.");
	apply_names(prompt, _chatState.user.name, _chatState.bot.name);

	PushMessage(Role::Director, 
		"{{" + prompt + "}}",
		MessageType::Direction,
		false);
	InstigateResponse(Responder::Bot, MessageType::Dialogue, 3);
	return true;
}

bool LLMInstance::InstigateResponse(Responder responder, MessageType msgType, int messageCount)
{
	if (!CanGenerate() || responder == Responder::None)
		return false;
	
	CancelGeneration();

	_atm_bGeneratingResponse.store(true);

	PrepareArguments prepareArgs {
		/*chat state*/ &_chatState,
		/*responder */ responder,
	};
	PrepareGeneration(prepareArgs);

	string responderName;
	Role role;
	switch (responder)
	{
	case Responder::Bot:
		responderName = _chatState.bot.name;
		role = Role::Bot;
		break;
	case Responder::User:
		responderName = _chatState.user.name;
		role = Role::User;
		break;
	case Responder::Narrator:
		responderName = name_from_role(Role::Narrator);
		role = Role::Narrator;
		break;
	case Responder::Director:
		responderName = name_from_role(Role::Director);
		role = Role::Director;
		break;
	}

	string prependMsg;
	if (msgType == MessageType::Dialogue)
		prependMsg = std::format("<{}=\"{}\">", Constants::DialogueTag, responderName);
	else if (msgType == MessageType::Action)
		prependMsg = std::format("<{}=\"{}\">", Constants::ActionTag, responderName);
	else if (msgType == MessageType::Thought)
		prependMsg = std::format("<{}=\"{}\">", Constants::ThoughtTag, responderName);
	else if (msgType == MessageType::Narration)
		prependMsg = std::format("<{}>", Constants::NarrationTag);
	else if (msgType == MessageType::Direction)
		prependMsg = std::format("<{}>", Constants::DirectionTag);
	else if (msgType == MessageType::UserMessage)
		prependMsg = std::format("<{}=\"{}\">", Constants::DialogueTag, responderName);

	GenerateArguments generateArgs {
		/*chat state*/ &_chatState,
		/*role*/ role,
		/*msgType*/ msgType,
		/*maxMessageCount*/ messageCount,
		/*prepend*/ prependMsg,
	};
	
	_workerThread = std::make_unique<std::jthread>(std::jthread(std::bind_front(&LLMInstance::Generate, this), generateArgs,
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
		}));

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
	if (!lock.owns_lock())
		return false;
//	std::scoped_lock lock(_resultMutex);

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
		if (!block.tokens.empty())
		{
			for (int32_t i = 0; i < block.length(); ++i)
				result.append(fnTokenStr(block.tokens[i]));
		}
		else
		{
			result.append(block.content);
		}
		result.append("]\r\n");
	}

	return WriteTextFile(filename, result, false);
}

bool LLMInstance::Reseed(uint32_t seed)
{
	if (!CanGenerate())
		return false;

	ModelState state = _atm_modelState.load();
	if (!state.bReady || !state.pModel)
		return false;

	llama_sampler* pChain = state.pSampler;
	int n = llama_sampler_chain_n(pChain);
	llama_sampler* pDistSampler = llama_sampler_chain_get(pChain, n - 1);
	if (pDistSampler)
	{
		llama_sampler_chain_remove(pChain, n - 1);
		llama_sampler_chain_add(pChain, llama_sampler_init_dist(seed));
		llama_sampler_reset(pChain);
		return true;
	}
	return false;
}

string LLMInstance::GetUserName() const
{
	return _chatState.user.name;
}

string LLMInstance::GetBotName() const
{
	return _chatState.bot.name;
}