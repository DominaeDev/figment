#include "llm/LLMInstance.h"
#include "llm/LLMUtility.h"
#include "util/StringUtility.h"
#include "util/Common.h"
#include "Constants.h"
#include <common.h>
#include <format>
#include <algorithm>
#include <assert.h>

#define DEBUG_SEED 0xA1B2C3D4

void ModelState::Release()
{
	if (pSampler)
	{
		SetActiveGrammar(Grammar::None); // Detach grammars (if any)
		llama_sampler_free(pSampler);
		llama_sampler_free(grammars[toI(Grammar::Default)]);
		llama_sampler_free(grammars[toI(Grammar::Continue)]);
	}
	if (pCtx)
	{
		llama_kv_self_clear(pCtx);
		llama_free(pCtx);
	}
	if (pModel)
		llama_model_free(pModel);

	pSampler = nullptr;
	pActiveGrammar = nullptr;
	grammars[toI(Grammar::Default)] = nullptr;	// Default grammar
	grammars[toI(Grammar::Continue)] = nullptr;	// Continue grammar
	pCtx = nullptr;
	pModel = nullptr;
	bReady = false;
	bInvalid = false;
}

llama_sampler* ModelState::SetActiveGrammar(Grammar grammar)
{
	if (static_cast<size_t>(grammar) >= grammars.size() 
		|| grammars[toI(Grammar::Default)] == nullptr)
		return nullptr;

	llama_sampler* pChain = pSampler;
	llama_sampler* pSelectedGrammar = grammars[toI(grammar)];

	if (pSelectedGrammar != nullptr && pSelectedGrammar == llama_sampler_chain_get(pChain, 0))
		return pSelectedGrammar; // No swap

	std::list<llama_sampler*> samplers;
	int32_t n = llama_sampler_chain_n(pChain);
	assert(n <= 5);

	for (int32_t i = n - 1; i >= 0; --i)
	{
		samplers.push_front(llama_sampler_chain_get(pChain, i));
		llama_sampler_chain_remove(pChain, i);
	}

	samplers.remove(grammars[toI(Grammar::Default)]);
	samplers.remove(grammars[toI(Grammar::Continue)]);
	pActiveGrammar = nullptr;

	if (pSelectedGrammar)
	{
		samplers.push_front(pSelectedGrammar);
		pActiveGrammar = pSelectedGrammar;
	}
	
	for (auto sampler : samplers)
		llama_sampler_chain_add(pChain, sampler);
	return pActiveGrammar;
}

int32_t ChatState::AssignBlockPositions()
{
	size_t pos = system_tokens.size();
	for (auto& block : blocks)
	{
		block.ctx_pos = (int32_t)pos;
		pos += block.length();
	}
	return (int32_t)pos;
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

	_atbGeneratingResponse.store(false);
	_atModelState.store(ModelState());
}

LLMInstance::~LLMInstance()
{
	Shutdown();
}

void LLMInstance::Shutdown()
{
	Halt();

	// Clear state and release
	auto state = _atModelState.exchange(ModelState());
	state.Release();

	PushStatus(LLMStatusSignal::ModelUnloaded);
}

using __LoadModelCallback = std::function<void(ModelState)>;
static LoadModelProgressCallback __LoadModelProgressCallback = nullptr; //! @thread-safety

static void OnLoadModelProgress(float progress, void* user_data)
{
	if (__LoadModelProgressCallback)
		__LoadModelProgressCallback(static_cast<int>(progress * 100.0f));
}


/// <summary>
/// Warning: Callin ctx_remove in isolation leaves the batch in an invalid state.
/// </summary>
static int32_t ctx_remove(llama_context* pCtx, ChatState& chat, std::vector<LLMMessageBlock>::iterator itBegin, std::vector<LLMMessageBlock>::iterator itEnd)
{
	llama_pos shift_amount = 0;
	for (auto it = itBegin; it != itEnd; ++it)
		shift_amount += (llama_pos)(*it).length();
	
	llama_pos pos_remove_begin = (*itBegin).ctx_pos;
	llama_pos pos_remove_end = pos_remove_begin + shift_amount;

	if (llama_kv_self_seq_rm(pCtx, 0, pos_remove_begin, pos_remove_end))
	{
		chat.blocks.erase(itBegin, itEnd);
		return shift_amount;
	}

	return 0; // Error
}

static int32_t ctx_remove_and_shift(llama_context* pCtx, ChatState& chat, std::vector<LLMMessageBlock>::iterator itBegin, std::vector<LLMMessageBlock>::iterator itEnd)
{
	// Remove
	llama_pos pos_remove_begin = (*itBegin).ctx_pos;
	int32_t shift_amount = ctx_remove(pCtx, chat, itBegin, itEnd);
	if (shift_amount == 0)
		return 0;
	llama_pos pos_remove_end = pos_remove_begin + shift_amount;

	// Shift
	llama_kv_self_seq_add(pCtx, 0, pos_remove_end, -1, -shift_amount);

	// Update batch
	auto& batch = chat.batch;
	int32_t n_batch = batch.n_tokens;
	for (int32_t i = 0; i < n_batch - pos_remove_end; ++i)
	{
		batch.token[pos_remove_begin + i] = batch.token[pos_remove_end + i];
		batch.logits[i] = false;
	}
	batch.n_tokens -= shift_amount;
	return (int32_t)-shift_amount;
}

static int32_t ctx_insert(llama_context* pCtx, llama_batch& batch, llama_pos pos_insert, LLMMessageBlock& block)
{
	// Where to insert in the existing context
	int32_t shift_amount = (int32_t)block.length();

	// Shift cache
	llama_kv_self_seq_add(pCtx, 0, pos_insert, -1, shift_amount);

	// Shift batch
	int32_t n_batch = batch.n_tokens;
	for (int32_t i = n_batch - 1; i >= pos_insert; --i)
	{
		batch.token[i + shift_amount] = batch.token[i];
		batch.logits[i + shift_amount] = batch.logits[i];
	}

	// Insert tokens into batch
	for (int32_t i = 0; i < shift_amount; ++i)
	{
		batch.token[pos_insert + i] = block.tokens[i];
		batch.logits[pos_insert + i] = false;
	}
	batch.n_tokens += shift_amount;

	block.ctx_pos = pos_insert;
	block.cached = true;
	return shift_amount;
}


bool LLMInstance::InitializeChat(string system_prompt, Messages messages)
{
	ModelState state = _atModelState.load();
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
		string grammar = ReadTextFile("./resources/grammar/formatting_grammar.gbnf").value_or("");
		string_util::replace_all(grammar, "##NAME_PATTERN##", "(\"" + _chatState.bot.name + "\")");
		llama_sampler* default_grammar_sampler = llama_sampler_init_grammar(pVocab, grammar.c_str(), "root");
		llama_sampler* continue_grammar_sampler = llama_sampler_init_grammar(pVocab, grammar.c_str(), "continue-root");
		if (default_grammar_sampler)
			DebugPrintLn("Grammar loaded");

		if (default_grammar_sampler) llama_sampler_chain_add(pSampler, default_grammar_sampler);	// Grammar
		llama_sampler_chain_add(pSampler, llama_sampler_init_min_p(0.15f, 1));						// Min P sampler
		llama_sampler_chain_add(pSampler, llama_sampler_init_temp(1.5f));							// Temperature
		llama_sampler_chain_add(pSampler, llama_sampler_init_penalties(512, 1.05f, 0.0f, 0.0f));	// Repeat penalty
#if _DEBUG
		llama_sampler_chain_add(pSampler, llama_sampler_init_dist(DEBUG_SEED));						// Seed
#else
		llama_sampler_chain_add(pSampler, llama_sampler_init_dist(LLAMA_DEFAULT_SEED));				// Seed
#endif

		state.pSampler = pSampler;
		state.pActiveGrammar = default_grammar_sampler;
		state.grammars[toI(Grammar::Default)] = default_grammar_sampler;
		state.grammars[toI(Grammar::Continue)] = continue_grammar_sampler;
		_atModelState.store(state);
	}

	// Init system prompt
	string prompt = string_util::trim(system_prompt);
	if (!string_util::empty_or_whitespace(_chatState.bot.description))
	{
		string persona;
		persona.reserve(_chatState.bot.description.size() + 20);
		persona.append("# About {{char}}:\n");
		persona.append(string_util::trim(_chatState.bot.description));
		string_util::replace(prompt, "##CHARACTER_INFO##", persona);
	}	
	if (!string_util::empty_or_whitespace(_chatState.user.description))
	{
		string user_persona;
		user_persona.reserve(_chatState.user.description.size() + 20);
		user_persona.append("# About {{user}}:\n");
		user_persona.append(string_util::trim(_chatState.user.description));
		string_util::replace(prompt, "##USER_INFO##", user_persona);
	}

	messages.insert(std::begin(messages), Message { Role::System, prompt });
	prompt = llm_util::apply_chat_template(messages, state.pCtx, false);

	llm_util::apply_names(prompt, _chatState.user.name, _chatState.bot.name);

	llama_context* pCtx = state.pCtx;

	// Tokenize system prompt
	_chatState.system_tokens = llm_util::tokenize(state.pModel, prompt, true);
	_chatState.current_pos = (int32_t)_chatState.system_tokens.size();
	_chatState.isInitialized = true;

	// Prepare assistant prelude
	string assistant_prefix = llm_util::apply_chat_template(Messages{}, state.pCtx, true);
	string_util::replace(assistant_prefix, "assistant", _chatState.bot.name);
	string_util::replace(assistant_prefix, "ASSISTANT", _chatState.bot.name);
	_chatState.assistant_tokens = llm_util::tokenize(state.pModel, assistant_prefix, false);

	// Pre-load system prompt into kv cache
	llama_kv_self_clear(pCtx);

	if (_chatState.batch.token != nullptr)
		llama_batch_free(_chatState.batch);

	// Prepare a batch for the prompt
	if (!llm_util::init_batch(state.pModel, state.pCtx, prompt, _chatState.batch))
		return false;

	if (_chatState.batch.n_tokens > 0 && llama_decode(state.pCtx, _chatState.batch))
	{
		fprintf(stderr, "failed to initialize chat\n");
		llama_batch_free(_chatState.batch);
		_chatState.batch = llama_batch {};
		return false;
	}

	// Initialize rng
#if _DEBUG
	_rng.seed(DEBUG_SEED);
#else
	_rng.seed((uint32_t)std::chrono::steady_clock::now().time_since_epoch().count());
#endif

	PushStatus(LLMStatusSignal::ChatStarted);
	return true;
}

bool LLMInstance::ResetChat(int seed)
{
	ModelState state = _atModelState.load();
	if (!CanGenerate())
		return false;

	llama_context* pCtx = state.pCtx;
	const int32_t maxCtx = llama_n_ctx(pCtx);

	// Reset batch pointer
		_chatState.current_pos = (int32_t)_chatState.system_tokens.size();
		_chatState.blocks.clear();
		_chatState.isInitialized = true;

	// Reinit the batch
	int32_t num_tokens = (int32_t)_chatState.system_tokens.size();
	llama_kv_self_seq_rm(state.pCtx, 0, num_tokens, -1);
	// llama_kv_self_clear(pCtx);

	// Add tokens to batch
	auto& batch = _chatState.batch;
	for (int i = 0; i < num_tokens; ++i)
	{
		batch.token[i] = _chatState.system_tokens[i];
		batch.pos[i] = i;
		batch.n_seq_id[i] = 1;
		batch.seq_id[i][0] = 0;
		batch.logits[i] = false; // No logits
	}
	batch.n_tokens = num_tokens;
	_chatState.current_pos = num_tokens;

	if (seed > 0)
		Reseed(seed);

	GreetUser();
	return true;
}

static void __LoadModel(string filename, __LoadModelCallback onComplete)
{
	const int ngl = 99; // All layers
	const int n_ctx = Constants::ContextSize;

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
			_atModelState.store(result);
			_modelName = string_util::get_filename(filename);
			PushStatus(LLMStatusSignal::ModelLoaded);
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
	ModelState state = _atModelState.load();
	return state.bReady && !state.bInvalid && state.pModel && _chatState.isInitialized;
}

bool LLMInstance::IsGenerating() const
{
	return _atbGeneratingResponse.load();
}

bool LLMInstance::CanGenerate() const
{
	return IsReady() && !IsGenerating();
}

bool LLMInstance::Continue(string responseId, string subMessageId)
{
	if (!CanGenerate())
		return false;

	if (_chatState.blocks.size() == 0)
		return false;

	LLMMessageBlock& block = _chatState.blocks[_chatState.blocks.size() - 1];
	if (block.responseId != responseId)
		return false; // Not last message

	auto [msgType, bComplete] = llm_util::detect_message_type(block.content);
	if (msgType == MessageType::Undefined || bComplete)
		return false; // Not incomplete message

	LLMMessageBlock blockCopy = block;
	blockCopy.cached = false;
	RemoveMessages(1); // Remove the last message, resets current_pos
	_chatState.blocks.push_back(blockCopy); // Reinsert block

	PrepareArguments prepareArgs {
		/*chat state*/ &_chatState,
		/*responder */ Responder::Continuation,
		/*time*/ 1,
	};
	PrepareGeneration(prepareArgs);
	_chatState.pre_response_pos = _chatState.current_pos; // Beginning of continued message
	
	GenerateArguments generateArgs {
		/*chat*/ &_chatState,
		/*role*/ block.role,
		/*msgType*/ msgType,
		/*maxMessageCount*/ 1,
		/*prepend*/ {},
		/*responseId*/ responseId,
		/*subMessageId*/ subMessageId,
		/*continue?*/ true,
	};
	StartGeneration(generateArgs);
	return true;
}

bool LLMInstance::Halt()
{
	if (!IsReady() || !IsGenerating())
		return false;

	CancelGeneration();
	_atbGeneratingResponse.store(false);

	return true;
}

void LLMInstance::PrepareGeneration(PrepareArguments args)
{
	// Load state
	ModelState state = _atModelState.load();
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

	// Decrement ttl
	for (int32_t i = (int32_t)chat.blocks.size() - 1; i >= 0; --i)
	{
		auto& block = chat.blocks[i];
		if (block.ttl <= 0)
			continue;
		block.ttl -= args.time;
		if (block.ttl > 0)
			continue;
		
		// Remove block
		if (!block.cached)
			chat.blocks.erase(std::begin(chat.blocks) + (ptrdiff_t)i);
		else
			current_pos += ctx_remove_and_shift(state.pCtx, _chatState, 
				std::begin(chat.blocks) + (ptrdiff_t)i, 
				std::begin(chat.blocks) + (ptrdiff_t)(toSZ(i + 1)));
	}

	// Tokenize uncached messages
	for (auto it = std::begin(chat.blocks); it != std::end(chat.blocks); ++it)
	{
		auto& block = *it;
		if (block.cached)
			continue;

		string content = block.content;
		if (args.responder == Responder::Continuation) // Continue response
			content = llm_util::apply_chat_template_prefix(Message { block.role, content }, userName, botName, state.pCtx, false);
		else
		{
			llm_util::complete_message(content);
			content = llm_util::apply_chat_template(Message { block.role, content }, state.pCtx, false);
			llm_util::apply_names(content, userName, botName);
		}
		
		auto block_tokens = llm_util::tokenize(state.pModel, content, false);
//		block.content = content;
		block.tokens = block_tokens;
		block.ctx_pos = 0; // assigned later
		prompt_tokens.insert(std::end(prompt_tokens), std::cbegin(block_tokens), std::cend(block_tokens));
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

		current_pos += ctx_remove_and_shift(state.pCtx, chat, std::begin(chat.blocks), std::begin(chat.blocks) + (ptrdiff_t)first_to_keep);
	}

	// Calculate block positions
	chat.AssignBlockPositions();

	// Store response position
	chat.pre_response_pos = current_pos + (int32_t)prompt_tokens.size();

	// Append assistant tokens
	if (args.responder == Responder::Bot)
		prompt_tokens.insert(std::end(prompt_tokens), std::begin(chat.assistant_tokens), std::end(chat.assistant_tokens));
	else if (args.responder != Responder::None && args.responder != Responder::Continuation)
	{
		string responderName;
		if (args.responder == Responder::Narrator)
			responderName = llm_util::name_from_role(Role::Narrator);
		else if (args.responder == Responder::User)
			responderName = chat.user.name;
		else
			responderName = chat.bot.name; // Fallback

		// Prepare assistant prelude
		string assistant_prelude = llm_util::apply_chat_template(Messages{}, state.pCtx, true);
		string_util::replace(assistant_prelude, "assistant", responderName);
		string_util::replace(assistant_prelude, "ASSISTANT", responderName);
		auto assistant_tokens = llm_util::tokenize(state.pModel, assistant_prelude, false);
		prompt_tokens.insert(std::end(prompt_tokens), std::begin(assistant_tokens), std::end(assistant_tokens));
	}

	// Store beginning of response (after assistant prelude)
	chat.prepend_pos = current_pos + (int32_t)prompt_tokens.size();

	// Append to batch
	for (int i = 0; i < prompt_tokens.size(); ++i)
		common_batch_add(batch, prompt_tokens[i], current_pos + i, { 0 }, false);
//	batch.logits[current_pos + prompt_tokens.size() - 1] = true;
	batch.n_tokens = current_pos + (int32_t)prompt_tokens.size();

	// Mark blocks in cache (they will be shortly)
	for (auto it = std::begin(chat.blocks); it != std::end(chat.blocks); ++it)
		it->cached = true;
}

void LLMInstance::__Generate(std::stop_token thread_stop, GenerateArguments args, __PartialResultCallback onPartial, __GenerationCompleteCallback onComplete)
{
	std::vector<llama_token> sampled_tokens;
	ModelState state = _atModelState.load();
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

	string responseId = args.responseId.empty() ? CreateUUID() : args.responseId;
	string subMessageId = args.subMessageId.empty() ? CreateUUID() : args.subMessageId;

	int numMessages = 0;
	string responderName {};

	DebugPrintLn(">> BEGIN GENERATION");

	// Init grammar
	if (auto pGrammar = state.SetActiveGrammar(args.bContinueLast ? Grammar::Continue : Grammar::Default))
		llama_sampler_reset(pGrammar);

	if (!args.prepend.empty())
	{
		auto prepend_tokens = llm_util::tokenize(state.pModel, args.prepend, false);

		// Append to batch
		for (int i = 0; i < prepend_tokens.size(); ++i)
			common_batch_add(batch, prepend_tokens[i], chat.prepend_pos + i, { 0 }, false);
		batch.logits[chat.prepend_pos + prepend_tokens.size() - 1] = true;

		partial += args.prepend;
		printf("%s", partial.c_str());
	}
	llm_util::init_batch_logits(batch);

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
		string str_token = llm_util::stringFromToken(pVocab, sampled_token);
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
		llm_util::process(partial, str_token, &bWait, &bHalt, stop_word);
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
					llm_util::get_tag_and_name(partial.substr(fmt_start, fmt_end - fmt_start + 1), tag, tagName);

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
					/*role*/ args.role,
					/*msgType*/ msgType,
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
				PushStatus(LLMStatusSignal::MessageComplete);
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

	llm_util::sanitize_response(response);

	if (response.length() > 0)
	{
		if (args.bContinueLast)
		{
			auto& lastBlock = chat.blocks[chat.blocks.size() - 1];
			lastBlock.content += response;
			lastBlock.tokens.insert(std::end(lastBlock.tokens), std::begin(sampled_tokens), std::end(sampled_tokens));
			lastBlock.cached = false;
		}
		else
		{
			chat.blocks.push_back(LLMMessageBlock {
				/*responseId*/ responseId,
				/*role*/ args.role,
				/*content*/ response,
				/*tokens*/ sampled_tokens,
				/*ctx_pos*/ pre_response_pos,
			});
		}
	}
	else
	{
		DebugPrint("(Empty response)");
	}

	DebugPrintLn();
	DebugPrintLn(std::format("END OF GENERATION (stopped on:{})", stop_word.c_str()));

	onComplete(InternalError::NoError, response);
};

void LLMInstance::StartGeneration(GenerateArguments args)
{
	static std::uniform_int_distribution<int> numMessages(1, 3);
	if (args.maxMessages <= 0) // Randomize number of messages
		args.maxMessages = numMessages(_rng);

	if (args.responseId.empty())
		args.responseId = CreateUUID();
	if (args.subMessageId.empty())
		args.subMessageId = CreateUUID();

	{
		std::scoped_lock(_resultMutex);
		_activeResponseIds.insert(args.responseId);
	}

	PushStatus(LLMStatusSignal::GenerationStarted);
	_atbGeneratingResponse.store(true);
	_workerThread = std::make_unique<std::jthread>(std::jthread(std::bind_front(&LLMInstance::__Generate, this), args,
		[](__PartialResult partial) {
			// ...
		},
		[this](InternalError error, string response) {
			// ...
			ModelState state = _atModelState.load();
			if (error != InternalError::NoError)
			{
				printf("\r\n>> Internal error: (%d) %s\r\n", error, response.c_str());
				state.bInvalid = true; // Invalidate state
				_atModelState.store(state);
			}

			_atbGeneratingResponse.store(false);
			PushStatus(LLMStatusSignal::GenerationComplete);

			// Update active responses
			{
				std::scoped_lock lock(_resultMutex);
				_activeResponseIds.clear();
				for (auto it = std::cbegin(_chatState.blocks); it != std::cend(_chatState.blocks); ++it)
					_activeResponseIds.insert(it->responseId);
			}
		}));
}

void LLMInstance::CancelGeneration()
{
	if (_workerThread.get() && _workerThread->joinable())
	{
		DebugPrint(">> Stopping worker thread ");
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

bool LLMInstance::SendMessage(string message)
{
	if (!CanGenerate())
		return false;

	if (string_util::empty_or_whitespace(message))
		return false;

	CancelGeneration();

	PushMessage(Role::User, message);

	PrepareArguments prepareArgs {
		/*chat state*/ &_chatState,
		/*responder */ Responder::Bot,
		/*time*/ 1,
	};
	PrepareGeneration(prepareArgs);

	GenerateArguments generateArgs {
		/*chat state*/ &_chatState,
		/*role*/ Role::Bot,
		/*msgType*/ MessageType::Undefined,
	};

	StartGeneration(generateArgs);

	return true;
}

bool LLMInstance::PushMessage(Role role, string message, MessageType msgType, bool visible, int ttl)
{
	if (!CanGenerate())
		return false;

	if (string_util::empty_or_whitespace(message))
		return false;

	// Process
	string name = llm_util::name_from_role(role);
	string content = message;
	llm_util::apply_names(content, _chatState.user.name, _chatState.bot.name);
	if (msgType == MessageType::SystemMessage)
		content = string_util::trim(content);
	else 
		content = llm_util::format_message(content, name);

	string responseId = CreateUUID();
	string subMessageId = CreateUUID();

	_chatState.blocks.push_back(LLMMessageBlock {
		/*blockId*/ responseId,
		/*role*/ role,
		/*content*/ content,
		/*tokens*/ {},
		/*ctx_pos*/ 0,
		/*cached*/ false,
		/*ttl*/ ttl > 0 ? ttl + 1 : 0,
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
			/*blockId*/ responseId,
			/*messageId*/ subMessageId,
			/*name*/ name,
			/*text*/ message,
			/*role*/ role,
			/*msgType*/ msgType,
			/*isComplete*/ true,
		});
		_activeResponseIds.insert(responseId);
	}
	return true;
}

std::vector<string> LLMInstance::RemoveMessages(int numMessages, bool rewindTime)
{
	if (!CanGenerate() || numMessages < 1)
		return {};

	size_t numRemovals = std::min(toSZ(numMessages), _chatState.blocks.size());
	size_t newSize = _chatState.blocks.size() - toSZ(numMessages);
	int32_t& current_pos = _chatState.current_pos;

	// Rewind time
	if (rewindTime)
	{
		for (auto& block : _chatState.blocks)
		{
			if (block.ttl > 0)
				block.ttl += toI(numRemovals);
		}
	}

	if (newSize > 0)
	{
		auto& block = _chatState.blocks[newSize - 1_sz];
		if (block.cached)
			current_pos = std::min(current_pos, block.ctx_pos + toI(block.length()));
		else
			current_pos = std::min(current_pos, block.ctx_pos);
	}
	else
	{
		current_pos = toI(_chatState.system_tokens.size());
	}
	
	// Update batch
	_chatState.batch.n_tokens = current_pos;

	// Clear kv cache
	ModelState state = _atModelState.load();
	llama_kv_self_seq_rm(state.pCtx, 0, current_pos, -1);

	// Return removed ids
	std::vector<string> removedIds;
	removedIds.reserve(_chatState.blocks.size() - (size_t)newSize);
	for (size_t i = (size_t)newSize; i < _chatState.blocks.size(); ++i)
		removedIds.push_back(_chatState.blocks[i].responseId);
	
	// Remove blocks
	_chatState.blocks.resize((size_t)newSize);
	return removedIds;
}

std::vector<string> LLMInstance::RollbackUserMessage()
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

	string greetingInstruction = "{{Greet {{user}} and let them know what you're thinking about right now.}}";
	llm_util::apply_names(greetingInstruction, _chatState.user.name, _chatState.bot.name);

	PushMessage(Role::Director, greetingInstruction, MessageType::Direction, false, 1);
	InstigateResponse(Responder::Bot, MessageType::Dialogue, 3);
	return true;
}

bool LLMInstance::Instruct(string instructions)
{
	if (!CanGenerate())
		return false;

	if (auto text = ReadTextFile("./resources/prompting/prompt_formatting_director.txt"))
	{
		string prompt = text.value();
		llm_util::apply_names(prompt, _chatState.user.name, _chatState.bot.name);
		PushMessage(Role::System, prompt, MessageType::SystemMessage, false, 1);
	}
	PushMessage(Role::Director, "{{" + instructions + "}}", MessageType::Direction, false, 4);
	InstigateResponse(Responder::Bot, MessageType::Undefined, 3);
	return true;
}

bool LLMInstance::InstigateResponse(Responder responder, MessageType msgType, int messageCount)
{
	if (!CanGenerate() 
		|| responder == Responder::None 
		|| responder == Responder::Continuation)
		return false;
	
	CancelGeneration();

	PrepareArguments prepareArgs {
		/*chat state*/ &_chatState,
		/*responder */ responder,
		/*time*/ 1,
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
		responderName = llm_util::name_from_role(Role::Narrator);
		role = Role::Narrator;
		break;
	case Responder::Director:
		responderName = llm_util::name_from_role(Role::Director);
		role = Role::Director;
		break;
	default:
		role = Role::Undefined;
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

	GenerateArguments generateArgs {
		/*chat state*/ &_chatState,
		/*role*/ role,
		/*msgType*/ msgType,
		/*maxMessageCount*/ messageCount,
		/*prepend*/ prependMsg,
	};
	
	StartGeneration(generateArgs);

	return true;
}


LLMStatus LLMInstance::GetStatus()
{
	ModelState state = _atModelState.load();

	LLMStatus status {};
	if (state.pModel && state.pCtx)
	{
		status.modelName = _modelName;
		status.allocCtxSize = llama_n_ctx(state.pCtx);
		status.usedCtxSize = llama_kv_self_used_cells(state.pCtx);
		status.bReady = state.bReady;
		status.bInvalid = state.bInvalid;
	}

	std::scoped_lock lock(_statusMutex);
	if (!_statusSignals.empty())
	{
		status.signal = _statusSignals.front();
		_statusSignals.pop();
	}
	return status;
}

bool LLMInstance::PollResponse(MessagePiece& piece)
{
	std::unique_lock<std::mutex> lock(_resultMutex, std::try_to_lock);
	if (!lock.owns_lock())
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

	ModelState state = _atModelState.load();
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
			return "\r\n"; // <NL>
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
	result.reserve(65536);

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

	ModelState state = _atModelState.load();
	if (!state.bReady || !state.pModel)
		return false;

	llama_sampler* pChain = state.pSampler;
	int n = llama_sampler_chain_n(pChain);
	llama_sampler* pDistSampler = llama_sampler_chain_get(pChain, n - 1);
	if (pDistSampler)
	{
		const char* sampler_name = llama_sampler_name(pDistSampler);
		if (strcmp(sampler_name, "dist") == 0)
		{
			llama_sampler_chain_remove(pChain, n - 1);
			llama_sampler_chain_add(pChain, llama_sampler_init_dist(seed));
			llama_sampler_reset(pChain);
		}

		_rng.seed(seed); // Use same seed
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

void LLMInstance::PushStatus(LLMStatusSignal signal)
{
	std::scoped_lock lock(_statusMutex);

	if (!_statusSignals.empty() && _statusSignals.back() == signal)
		return;

	_statusSignals.push(signal);
}

std::set<string> LLMInstance::GetActiveMessages()
{
	std::scoped_lock lock(_resultMutex);
	return std::set<string>(std::begin(_activeResponseIds), std::end(_activeResponseIds)); // Copy
}