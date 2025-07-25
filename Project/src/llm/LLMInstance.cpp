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
#define LIMIT_MSG_COUNT 1
#define RANDOMIZE_MSG_COUNT 1

void ModelState::Release()
{
	if (pSampler)
	{
		SetActiveGrammar(Grammar::None); // Detach grammar (if any)
		llama_sampler_free(pSampler);
	}

	for (size_t i = 0; i < grammars.size(); ++i)
	{
		llama_sampler_free(grammars[i]);
		grammars[i] = nullptr;
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
	pCtx = nullptr;
	pModel = nullptr;
}

llama_sampler* ModelState::SetActiveGrammar(Grammar grammar)
{
	if (static_cast<size_t>(grammar) >= grammars.size() || grammars[castEnum(Grammar::Default)] == nullptr)
		return nullptr;

	llama_sampler* pChain = pSampler;
	llama_sampler* pSelectedGrammar = grammars[castEnum(grammar)];

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

	for (size_t i = 0; i < grammars.size(); ++i)
		samplers.remove(grammars[i]);
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

int32_t ContextState::AssignBlockPositions()
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
}

LLMInstance::~LLMInstance()
{
	Shutdown();
}

void LLMInstance::Shutdown()
{
	Halt(); 
		
	{	// Clear and release state
		std::scoped_lock lock(_stateMutex);
		_modelState.Release();
		_modelState = {};
		_contextState = {};
	}
	_readyState.store(ReadyState::Uninitialized);
	PushSignal(LLMStatusSignal::UnloadedModel);
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
static int32_t ctx_remove(llama_context* pCtx, ContextState& chat, std::vector<ContextBlock>::iterator itBegin, std::vector<ContextBlock>::iterator itEnd)
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

static int32_t ctx_remove_and_shift(llama_context* pCtx, ContextState& chat, std::vector<ContextBlock>::iterator itBegin, std::vector<ContextBlock>::iterator itEnd)
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

static int32_t ctx_insert(llama_context* pCtx, llama_batch& batch, llama_pos pos_insert, ContextBlock& block)
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
	std::scoped_lock lock(_stateMutex);

	if (_readyState < ReadyState::ModelLoaded || !_modelState.pModel)
		return false;

	PushSignal(LLMStatusSignal::InitializingChat);

	_contextState = {};

	user.LoadFromXml("characters/user.xml"); //! tmp
	bot.LoadFromXml("characters/character.xml"); //! tmp

	// Initialize sampler + grammar
	const llama_vocab* pVocab = llama_model_get_vocab(_modelState.pModel);

	// Init sampler chain
	if (_modelState.pSampler == nullptr)
	{
		llama_sampler_chain_params sampler_params = llama_sampler_chain_default_params();
		llama_sampler* pSampler = llama_sampler_chain_init(sampler_params);

		// Load grammar(s)
		string grammar = ReadTextFile("./resources/grammar/formatting_grammar.gbnf").value_or("");
		string_util::replace_all(grammar, "##NAME_PATTERN##", "(\"" + bot.name + "\")");
		llama_sampler* default_grammar_sampler = llama_sampler_init_grammar(pVocab, grammar.c_str(), "root");
		if (default_grammar_sampler)
		{
			_modelState.grammars[castEnum(Grammar::Default)]				= default_grammar_sampler;
			_modelState.grammars[castEnum(Grammar::StubDialogue)]			= llama_sampler_init_grammar(pVocab, grammar.c_str(), "stub-talk");
			_modelState.grammars[castEnum(Grammar::StubAction)]			= llama_sampler_init_grammar(pVocab, grammar.c_str(), "stub-act");
			_modelState.grammars[castEnum(Grammar::StubNarration)]		= llama_sampler_init_grammar(pVocab, grammar.c_str(), "stub-narr");
			_modelState.grammars[castEnum(Grammar::ContinueDialogue)]		= llama_sampler_init_grammar(pVocab, grammar.c_str(), "cont-talk");
			_modelState.grammars[castEnum(Grammar::ContinueAction)]		= llama_sampler_init_grammar(pVocab, grammar.c_str(), "cont-act");
			_modelState.grammars[castEnum(Grammar::ContinueNarration)]	= llama_sampler_init_grammar(pVocab, grammar.c_str(), "cont-narr");

			DebugPrintLn("Grammar loaded");
		}

		if (default_grammar_sampler) llama_sampler_chain_add(pSampler, default_grammar_sampler);	// Grammar
		llama_sampler_chain_add(pSampler, llama_sampler_init_min_p(0.15f, 1));						// Min P sampler
		llama_sampler_chain_add(pSampler, llama_sampler_init_temp(1.5f));							// Temperature
		llama_sampler_chain_add(pSampler, llama_sampler_init_penalties(512, 1.05f, 0.0f, 0.0f));	// Repeat penalty
#if _DEBUG
		llama_sampler_chain_add(pSampler, llama_sampler_init_dist(DEBUG_SEED));						// Seed
#else
		llama_sampler_chain_add(pSampler, llama_sampler_init_dist(LLAMA_DEFAULT_SEED));				// Seed
#endif

		_modelState.pSampler = pSampler;
		_modelState.pActiveGrammar = default_grammar_sampler;
	}

	// Init system prompt
	string prompt = string_util::trim(system_prompt);
	if (!string_util::empty_or_whitespace(bot.description))
	{
		string persona;
		persona.reserve(bot.description.size() + 20);
		persona.append("# About {{char}}:\n");
		persona.append(string_util::trim(bot.description));
		string_util::replace(prompt, "##CHARACTER_INFO##", persona);
	}	
	if (!string_util::empty_or_whitespace(user.description))
	{
		string user_persona;
		user_persona.reserve(user.description.size() + 20);
		user_persona.append("# About {{user}}:\n");
		user_persona.append(string_util::trim(user.description));
		string_util::replace(prompt, "##USER_INFO##", user_persona);
	}

	messages.insert(std::begin(messages), Message { Role::System, prompt });
	prompt = llm_util::apply_chat_template(messages, _modelState.pCtx, false);

	llm_util::apply_names(prompt, user.name, bot.name);

	llama_context* pCtx = _modelState.pCtx;

	// Tokenize system prompt
	_contextState.system_tokens = llm_util::tokenize(_modelState.pModel, prompt, true);
	_contextState.current_pos = (int32_t)_contextState.system_tokens.size();

	// Prepare assistant prelude
//	string assistant_prefix = llm_util::apply_chat_template(Messages{}, _modelState.pCtx, true);
//	string_util::replace(assistant_prefix, "assistant", bot.name);
//	string_util::replace(assistant_prefix, "ASSISTANT", bot.name);
//	_contextState.assistant_tokens = llm_util::tokenize(_modelState.pModel, assistant_prefix, false);

	// Pre-load system prompt into kv cache
	llama_kv_self_clear(pCtx);

	if (_contextState.batch.token != nullptr)
		llama_batch_free(_contextState.batch);

	// Prepare a batch for the prompt
	if (!llm_util::init_batch(_modelState.pModel, _modelState.pCtx, prompt, _contextState.batch))
	{
		_readyState.store(ReadyState::Invalid);
		PushSignal(LLMStatusSignal::InitializeChatFailure);
		return false;
	}

	if (_contextState.batch.n_tokens > 0 && llama_decode(_modelState.pCtx, _contextState.batch))
	{
		fprintf(stderr, "failed to initialize chat\n");
		llama_batch_free(_contextState.batch);
		_contextState.batch = llama_batch {};

		_readyState.store(ReadyState::Invalid);
		PushSignal(LLMStatusSignal::InitializeChatFailure);
		return false;
	}

	// Initialize rng
#if _DEBUG
	_modelState.rng.seed(DEBUG_SEED);
#else
	_modelState.rng.seed((uint32_t)std::chrono::steady_clock::now().time_since_epoch().count());
#endif

	_readyState.store(ReadyState::Ready);
	PushSignal(LLMStatusSignal::InitializedChat);
	return true;
}

bool LLMInstance::ResetChat(int seed)
{
	if (!IsReady())
		return false;

	if (_readyState.load() == ReadyState::Generating)
		Halt(); // Cancel ongoing generation

	{	// Lock state
		std::scoped_lock lock(_stateMutex, _resultMutex);

		llama_context* pCtx = _modelState.pCtx;
		const int32_t maxCtx = llama_n_ctx(pCtx);

		// Reset batch pointer
		_contextState.current_pos = (int32_t)_contextState.system_tokens.size();
		_contextState.blocks.clear();

		// Reinit the batch
		int32_t num_tokens = (int32_t)_contextState.system_tokens.size();
		llama_kv_self_seq_rm(_modelState.pCtx, 0, num_tokens, -1);
		// llama_kv_self_clear(pCtx);

		// Add tokens to batch
		auto& batch = _contextState.batch;
		for (int i = 0; i < num_tokens; ++i)
		{
			batch.token[i] = _contextState.system_tokens[i];
			batch.pos[i] = i;
			batch.n_seq_id[i] = 1;
			batch.seq_id[i][0] = 0;
			batch.logits[i] = false; // No logits
		}
		batch.n_tokens = num_tokens;
		_contextState.current_pos = num_tokens;
		
		ClearQueue(_resultQueue);
		_readyState.store(ReadyState::Ready);
	}

	if (seed > 0)
		Reseed(seed);

	PushSignal(LLMStatusSignal::InitializedChat);

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
	state.modelName = string_util::get_filename(filename);

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

	onComplete(state);
}

bool LLMInstance::LoadModelAsync(string filename, LoadModelProgressCallback onProgress, LoadModelCallback onComplete)
{
	if (IsReady())
		return false; // Already loaded

	_readyState.store(ReadyState::LoadingModel);
	__LoadModelProgressCallback = onProgress;

	_workerThread = std::make_unique<std::jthread>(std::jthread(__LoadModel, filename,
		[this, filename, onComplete](ModelState result)
	{
		if (result.pModel) // Success
		{
			{	// Write state
				std::scoped_lock lock(_stateMutex);
				_modelState = result;
			}
			_readyState.store(ReadyState::ModelLoaded);

			PushSignal(LLMStatusSignal::LoadedModel);
			onComplete(true);
		}
		else // Failure
		{
			result.Release();

			PushSignal(LLMStatusSignal::LoadModelFailure);
			onComplete(false);
		}
	}));

	return true;
}

bool LLMInstance::IsReady() const
{
	return _readyState.load() >= ReadyState::Ready;
}

bool LLMInstance::IsGenerating() const
{
	return _readyState.load() == ReadyState::Generating;
}

bool LLMInstance::CanGenerate() const
{
	return _readyState.load() == ReadyState::Ready;
}

bool LLMInstance::Continue(string responseId, string subMessageId, bool extend)
{
	if (!CanGenerate())
		return false;

	GenerateArguments generateArgs;

	{	// Acquire state lock
		std::unique_lock lock(_stateMutex);

		if (_contextState.blocks.size() == 0)
			return false;

		ContextBlock& currBlock = _contextState.blocks[_contextState.blocks.size() - 1];
		if (currBlock.responseId != responseId)
			return false; // Not last message

		auto [msgType, bComplete] = llm_util::detect_message_type(currBlock.content);
		if (msgType == MessageType::Undefined || (bComplete && !extend))
			return false; // Not incomplete message

		ContextBlock block = currBlock;
		block.cached = false;
		impl_RemoveMessages(1, true); // Remove the last message, resets current_pos

		if (extend && bComplete)
		{
			// Strip end tag
			size_t pos_end = block.content.rfind("</", std::string::npos);
			if (pos_end != std::string::npos)
			{
				block.content = block.content.substr(0, pos_end);
				char last_char = block.content.back();
				if (last_char == '*' || last_char == '"' || last_char == ']')
					block.content.pop_back(); // Trim scaffolding char
			}
		}
		_contextState.blocks.push_back(block); // Reinsert block
		_contextState.pre_response_pos = _contextState.current_pos; // Beginning of continued message
		
		generateArgs = GenerateArguments {
			/*role*/ block.role,
			/*msgType*/ msgType,
			/*flag*/ GenerateFlag::Continuation,
			/*maxMessageCount*/ 1,
			/*prepend*/ {},
			/*responseId*/ responseId,
			/*subMessageId*/ subMessageId,
		};
	}

	PrepareArguments prepareArgs {
		/*responder */ Responder::None,
		/*time*/ 1,
	};
	PrepareGeneration(prepareArgs);
	
	StartGeneration(generateArgs);
	return true;
}

bool LLMInstance::Halt()
{
	if (_readyState.load() != ReadyState::Generating)
		return false;

	// Cancel and join worker thread
	if (_workerThread.get() && _workerThread->joinable())
	{
		_workerThread->request_stop();
		_workerThread->join();
	}
	_readyState.store(ReadyState::Ready);
	return true;
}

void LLMInstance::PrepareGeneration(PrepareArguments args)
{
	// Load state
	std::scoped_lock lock(_stateMutex);

	ModelState& state = _modelState;
	ContextState& chat = _contextState;
	llama_batch& batch = chat.batch;
	const llama_vocab* pVocab = llama_model_get_vocab(state.pModel);

	string userName = user.name;
	string botName = bot.name;

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
			current_pos += ctx_remove_and_shift(state.pCtx, _contextState, 
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
		if (args.responder == Responder::None) // Continue response
			content = llm_util::apply_chat_template_prefix(Message { block.role, content }, userName, botName, state.pCtx, false);
		else
		{
			llm_util::complete_message(content);
			content = llm_util::apply_chat_template(Message { block.role, content }, state.pCtx, false);
			llm_util::apply_names(content, userName, botName);
		}
		
		auto block_tokens = llm_util::tokenize(state.pModel, content, false);
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
	if (args.responder != Responder::None)
	{
		string prelude = llm_util::get_responder_prelude(args.responder, state.pCtx);
		llm_util::apply_names(prelude, userName, botName);
		auto assistant_tokens = llm_util::tokenize(state.pModel, prelude, false);
		prompt_tokens.insert(std::end(prompt_tokens), std::begin(assistant_tokens), std::end(assistant_tokens));
	}

	// Store beginning of response (after assistant prelude)
	chat.prepend_pos = current_pos + (int32_t)prompt_tokens.size();

	// Append to batch
	for (int i = 0; i < prompt_tokens.size(); ++i)
		common_batch_add(batch, prompt_tokens[i], current_pos + i, { 0 }, false);
	batch.n_tokens = current_pos + (int32_t)prompt_tokens.size();

	// Mark blocks in cache (they will be shortly)
	for (auto it = std::begin(chat.blocks); it != std::end(chat.blocks); ++it)
		it->cached = true;
}

void LLMInstance::__Generate(std::stop_token thread_stop, GenerateArguments args, __PartialResultCallback onPartial, __GenerationCompleteCallback onComplete)
{
	std::unique_lock stateLock(_stateMutex); // Acquire state lock

	std::vector<llama_token> sampled_tokens;
	ModelState& state = _modelState;
	const llama_vocab* pVocab = llama_model_get_vocab(state.pModel);

	llama_token sampled_token;
	string partial;
	string stop_word;
	string response;
	MessageType msgType = args.msgType;
	bool isContinuation = args.flags == GenerateFlag::Continuation;
	bool isInstigation = args.flags == GenerateFlag::Instigation;

	ContextState& chat = _contextState;
	llama_batch& batch = chat.batch;
	int32_t n_batch = llama_n_batch(state.pCtx);
	int32_t ctx_size  = llama_n_ctx(state.pCtx);
	int32_t& current_pos = chat.current_pos;
	string userName = user.name;
	string botName = bot.name;
	int32_t pre_response_pos = chat.pre_response_pos;

	string responseId = args.responseId.empty() ? CreateUUID() : args.responseId;
	string subMessageId = args.subMessageId.empty() ? CreateUUID() : args.subMessageId;

	int numMessages = 0;
	string responderName {};

	DebugPrintLn(">> BEGIN GENERATION");

	// Select and init grammar
	Grammar grammar = Grammar::None;
	if (args.msgType != MessageType::Undefined)
	{
		if (isContinuation)
		{
			if (args.msgType == MessageType::Dialogue)
				grammar = Grammar::ContinueDialogue;
			else if (args.msgType == MessageType::Action)
				grammar = Grammar::ContinueAction;
			else if (args.msgType == MessageType::Narration)
				grammar = Grammar::ContinueNarration;
		}
		else if (isInstigation)
		{
			if (args.msgType == MessageType::Dialogue)
				grammar = Grammar::StubDialogue;
			else if (args.msgType == MessageType::Action)
				grammar = Grammar::StubAction;
			else if (args.msgType == MessageType::Narration)
				grammar = Grammar::StubNarration;
		}
		else
			grammar = Grammar::Default;
	}
	else
		grammar = Grammar::Default;

	if (auto pGrammar = state.SetActiveGrammar(grammar))
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

	stateLock.unlock();
	auto startTime = std::chrono::steady_clock::now();

	while (true)
	{
		bool next_token = true;

		if (thread_stop.stop_requested())
			break; // Cancelled
		if (current_pos >= ctx_size)
			break; // Max limit reached

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

		bool bSend = true;
		bool bHalt = false;
		bool bWait = false;
		string str_token;
		if (!llama_vocab_is_eog(pVocab, sampled_token))
		{
			// convert the token to a string, print it and add it to the response
			str_token = llm_util::stringFromToken(pVocab, sampled_token);
			if (str_token.size() == 0)
				break; // Error

			partial += str_token;
			sampled_tokens.push_back(sampled_token);

			// check if there is incomplete UTF-8 character at the end
			bHalt = false;
			bWait = false;
			llm_util::process(partial, str_token, &bWait, &bHalt, stop_word);
		}
		else // EOG token
		{
			bHalt = true;
			stop_word = "EOG";
		}

		if (sampled_tokens.size() >= Constants::MaxResponseLength)
		{
			bHalt = true;
			stop_word = "length";
		}

		bSend &= !bWait;
		next_token &= !bHalt;

		if (bSend)
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
					{
						stop_word = "user";
						break; // Stop if talking/acting for the user
					}

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
				{
					stop_word = "msg count";
					break; // That's enough, thank you
				}
				subMessageId = CreateUUID();
				PushSignal(LLMStatusSignal::CompletedMessage);
			}
		}

		// Print to console
		printf("%s", str_token.c_str());

		common_batch_add(batch, sampled_token, current_pos, { 0 }, true);

		// prepare the next batch with the sampled token
		if (!next_token)
			break; // TODO: Carry over?
	}
	fflush(stdout);

	auto endTime = std::chrono::steady_clock::now();
	if (!sampled_tokens.empty())
	{
		double duration = toD(std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count());
		_tokensPerSec.store(toD(sampled_tokens.size()) / (duration / 1000.0));
	}

	stateLock.lock();

	// Remove full response from cache (re-added, with formatting, next generation)
	llama_kv_self_seq_rm(state.pCtx, 0, pre_response_pos, -1);
	batch.n_tokens = pre_response_pos;
	chat.current_pos = pre_response_pos;

	llm_util::sanitize_response(response);

	if (response.length() > 0)
	{
		if (isContinuation)
		{
			auto& lastBlock = chat.blocks[chat.blocks.size() - 1];
			lastBlock.content += response;
			lastBlock.tokens.insert(std::end(lastBlock.tokens), std::begin(sampled_tokens), std::end(sampled_tokens));
			lastBlock.cached = false;
		}
		else
		{
			chat.blocks.push_back(ContextBlock {
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
	DebugPrintLn(std::format("END OF GENERATION (stopped on:{}) [{}]", stop_word.c_str(), sampled_tokens.size()));

	stateLock.unlock();
	onComplete(InternalError::NoError, response);
};

void LLMInstance::StartGeneration(GenerateArguments args)
{
#if defined(_DEBUG) && RANDOMIZE_MSG_COUNT
	{	// Acquire state lock
		std::scoped_lock lock(_stateMutex);
		static std::uniform_int_distribution<int> numMessages(1, 3);
		if (args.maxMessages <= 0) // Randomize number of messages
			args.maxMessages = numMessages(_modelState.rng);
	}
#else !LIMIT_MSG_COUNT
	args.maxMessages = 0;
#endif

	if (args.responseId.empty())
		args.responseId = CreateUUID();
	if (args.subMessageId.empty())
		args.subMessageId = CreateUUID();

	{
		std::scoped_lock(_resultMutex);
		_activeResponseIds.insert(args.responseId);
	}

	PushSignal(LLMStatusSignal::GenerationStarted);
	_readyState.store(ReadyState::Generating);

	_workerThread = std::make_unique<std::jthread>(std::jthread(std::bind_front(&LLMInstance::__Generate, this), args,
		[](__PartialResult partial) {
			// ...
		},
		[this](InternalError error, string response) {
			// ...
			if (error != InternalError::NoError)
			{
				printf("\r\n>> Internal error: (%d) %s\r\n", error, response.c_str());
				_readyState.store(ReadyState::Invalid);
			}
			else
			{
				_readyState.store(ReadyState::Ready);
			}

			RefreshActiveResponses();

			PushSignal(LLMStatusSignal::GenerationComplete);
		}));
}

void LLMInstance::ClearResponseQueue()
{
	std::scoped_lock lock(_resultMutex);
	ClearQueue(_resultQueue);
}

bool LLMInstance::SendMessage(string message)
{
	if (!CanGenerate())
		return false;

	if (string_util::empty_or_whitespace(message))
		return false;

	PushMessage(Role::User, message);

	PrepareArguments prepareArgs {
		/*responder */ Responder::Bot,
		/*time*/ 1,
	};
	PrepareGeneration(prepareArgs);

	GenerateArguments generateArgs {
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
	llm_util::apply_names(content, user.name, bot.name);
	std::vector<Submessage> subMessages;
	content = llm_util::process_message(content, name, &subMessages);

	if (msgType == MessageType::Undefined)
		msgType = llm_util::detect_message_type(content).first;
	
	if (msgType == MessageType::SystemMessage) //! @correctness
		role = Role::System;
	else if (msgType == MessageType::Narration)
		role = Role::Narrator;
	else if (msgType == MessageType::Direction)
		role = Role::Director;

	string responseId = CreateUUID();

	{	// Acquire state lock
		std::scoped_lock lock(_stateMutex);
		_contextState.blocks.push_back(ContextBlock {
			/*blockId*/ responseId,
			/*role*/ role,
			/*content*/ content,
			/*tokens*/ {},
			/*ctx_pos*/ 0,
			/*cached*/ false,
			/*ttl*/ ttl > 0 ? ttl + 1 : 0,
		});
	}

	if (visible)
	{
		if (role == Role::Bot)
			name = bot.name;
		else if (role == Role::User)
			name = user.name;
		else
			name = llm_util::name_from_role(role);

		std::scoped_lock lock(_resultMutex);
		_activeResponseIds.insert(responseId);
		
		// Add message to result queue
		for (auto subMsg : subMessages)
		{
			string subMessageId = CreateUUID();
			_resultQueue.push(MessagePiece {
				/*blockId*/ responseId,
				/*messageId*/ subMessageId,
				/*name*/ name,
				/*text*/ subMsg.content,
				/*role*/ role,
				/*msgType*/ subMsg.msgType,
				/*isComplete*/ true,
				});
		}
	}
	else
	{
		std::scoped_lock lock(_resultMutex);
		_activeResponseIds.insert(responseId);
	}
	return true;
}

std::vector<RemovedMessage> LLMInstance::RemoveMessages(int numMessages, bool rewindTime)
{
	if (!CanGenerate() || numMessages < 1)
		return {};

	std::scoped_lock lock(_stateMutex);

	return impl_RemoveMessages(numMessages, rewindTime);
}

std::vector<RemovedMessage> LLMInstance::impl_RemoveMessages(int numMessages, bool rewindTime)
{
	size_t numRemovals = std::min(toSZ(numMessages), _contextState.blocks.size());
	size_t newSize = toSZ(std::max(toI(_contextState.blocks.size()) - numMessages, 0));
	int32_t& current_pos = _contextState.current_pos;

	// Rewind time
	if (rewindTime)
	{
		for (auto& block : _contextState.blocks)
		{
			if (block.ttl > 0)
				block.ttl += toI(numRemovals);
		}
	}

	if (newSize > 0)
	{
		auto& block = _contextState.blocks[newSize - 1_sz];
		if (block.cached)
			current_pos = std::min(current_pos, block.ctx_pos + toI(block.length()));
		else
			current_pos = std::min(current_pos, block.ctx_pos);
	}
	else
	{
		current_pos = toI(_contextState.system_tokens.size());
	}
	
	// Update batch
	_contextState.batch.n_tokens = current_pos;

	// Clear kv cache
	ModelState& state = _modelState;
	llama_kv_self_seq_rm(state.pCtx, 0, current_pos, -1);

	// Return removed ids
	std::vector<RemovedMessage> removedIds;
	removedIds.reserve(_contextState.blocks.size() - (size_t)newSize);
	for (size_t i = (size_t)newSize; i < _contextState.blocks.size(); ++i)
	{
		auto const& block = _contextState.blocks[i];
		removedIds.push_back(RemovedMessage {
			block.responseId,
			block.content,
			block.role,
		});
	}
	
	// Remove blocks
	_contextState.blocks.resize((size_t)newSize);
	return removedIds;
}

std::vector<RemovedMessage> LLMInstance::RollbackUserMessage()
{
	if (!CanGenerate())
		return {};

	std::scoped_lock lock(_stateMutex);

	for (int i = (int32_t)_contextState.blocks.size() - 1; i >= 0; --i)
	{
		if (_contextState.blocks[i].role == Role::User)
			return impl_RemoveMessages((int32_t)_contextState.blocks.size() - i, true);
	}
	return {};
}

bool LLMInstance::GreetUser()
{
	if (!CanGenerate())
		return false;

	string greetingInstruction = "{{Greet {{user}} and let them know what you're thinking about right now.}}";
	llm_util::apply_names(greetingInstruction, user.name, bot.name);

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
		llm_util::apply_names(prompt, user.name, bot.name);
		PushMessage(Role::System, prompt, MessageType::SystemMessage, false, 1);
	}

	PushMessage(Role::Director, "{{" + instructions + "}}", MessageType::Direction, false, 4);
	InstigateResponse(Responder::Bot, MessageType::Undefined, 3);
	return true;
}

bool LLMInstance::InstigateResponse(Responder responder, MessageType msgType, int messageCount)
{
	if (!CanGenerate() || responder == Responder::None)
		return false;
	
	PrepareArguments prepareArgs {
		/*responder */ responder,
		/*time*/ 1,
	};
	PrepareGeneration(prepareArgs);

	string responderName;
	Role role;
	switch (responder)
	{
	case Responder::Bot:
		responderName = bot.name;
		role = Role::Bot;
		break;
	case Responder::User:
		responderName = user.name;
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
		/*role*/ role,
		/*msgType*/ msgType,
		/*flag*/ GenerateFlag::Instigation,
		/*maxMessageCount*/ messageCount,
		/*prepend*/ prependMsg,
	};
	
	StartGeneration(generateArgs);
	return true;
}

std::pair<LLMStatus, bool> LLMInstance::PollStatus()
{
	LLMStatus status {};

	// Try to acquire state lock
	std::unique_lock<std::mutex> lock(_stateMutex, std::try_to_lock);
	if (lock.owns_lock())
	{
		if (_modelState.pModel && _modelState.pCtx)
		{
			status.modelName = _modelState.modelName;
			status.allocCtxSize = llama_n_ctx(_modelState.pCtx);
			status.usedCtxSize = llama_kv_self_used_cells(_modelState.pCtx);
			status.tokensPerSec = _tokensPerSec.load();
		}
		lock.unlock();
		lock.release();
	}

	ReadyState readyState = _readyState.load();
	status.bReady = readyState >= ReadyState::Ready;
	status.bInvalid = readyState == ReadyState::Invalid;

	{	// Acquire signal lock
		std::scoped_lock signalLock(_statusMutex);
		if (!_statusSignals.empty())
		{
			status.signal = _statusSignals.front();
			_statusSignals.pop();
		}
	}
	return std::make_pair(status, true);
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

	const ModelState& state = _modelState; //! @threading Don't acquire lock since we're only using this for debugging.
	const llama_vocab* pVocab = llama_model_get_vocab(state.pModel);
	const llama_batch& batch = _contextState.batch;

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

	// Detokenize the batched tokens
	string result;
	result.reserve(65536);

	for (int32_t i = 0; i < batch.n_tokens; ++i)
		result.append(fnTokenStr(batch.token[i]));

	for (auto& block : _contextState.blocks)
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

	std::scoped_lock lock(_stateMutex);
	ModelState& state = _modelState;

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

		state.rng.seed(seed); // Use same seed
		return true;
	}

	return false;
}

string LLMInstance::GetUserName() const
{
	return user.name;
}

string LLMInstance::GetBotName() const
{
	return bot.name;
}

void LLMInstance::PushSignal(LLMStatusSignal signal)
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

void LLMInstance::RefreshActiveResponses()
{
	std::scoped_lock lock(_resultMutex);
	_activeResponseIds.clear();
	for (auto it = std::cbegin(_contextState.blocks); it != std::cend(_contextState.blocks); ++it)
		_activeResponseIds.insert(it->responseId);
}