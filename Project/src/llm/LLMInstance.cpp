#include "llm/LLMInstance.h"
#include "llm/LLMUtility.h"
#include "llm/LLMTemplate.h"
#include "llm/Embedding.h"
#include "util/StringUtility.h"
#include "util/Common.h"
#include <common.h>
#include <format>
#include <algorithm>
#include <cassert>
#include <format>
#include <chrono>

#define DEBUG_SEED 0xA1B2C3D4

using namespace std::chrono_literals;

void ModelState::Release()
{
	if (pSampler)
	{
		SetActiveGrammar(GrammarFlag::None); // Detach grammar (if any)
		llama_sampler_free(pSampler);
	}

	for (auto& kvp : grammars)
		llama_sampler_free(kvp.second);
	grammars.clear();

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
	pVocab = nullptr;
}

llama_sampler* ModelState::SetActiveGrammar(GrammarFlag flags)
{
	llama_sampler* pSelectedGrammar = nullptr;
	if (flags != GrammarFlag::None)
	{
		auto itFind = grammars.find(flags);
		if (itFind != grammars.end())
			pSelectedGrammar = itFind->second;
	}
	
	llama_sampler* pChain = pSampler;
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

	samplers.remove(pActiveGrammar);
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

bool ModelState::HasGrammar(GrammarFlag flags) const
{
	return grammars.find(flags) != grammars.end();
}

int32_t ContextState::AssignBlockPositions()
{
	int32_t offset = 0;
	for (auto& block : blocks)
	{
		block.offset = offset;
		offset += block.length();
	}
	return offset;
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

		if (_pEmbedding)
		{
			_pEmbedding->Shutdown();
			_pEmbedding = nullptr;
		}
	}
	_readyState.store(ReadyState::Uninitialized);
	PushSignal(LLMStatusSignal::UnloadedModel);

	llama_backend_free();
}

using __LlamaLogCallback = std::function<void(ggml_log_level level, const char* text, void* user_data)>;
static void OnLlamaLog(ggml_log_level level, const char* text, void* user_data)
{
	LLMInstance* pThis = static_cast<LLMInstance*>(user_data);

	string log(text);
	size_t pos_eq = log.find('=');
	bool bGPU = log.find("CUDA") != string::npos;
	bool bCPU = log.find("CPU") != string::npos;

	if (pos_eq != string::npos && log.find("buffer size") != string::npos)
	{
		string value = string_util::trim(log.substr(pos_eq + 1));
		double mul= 1024.0 * 1024.0; // MiB
		if (string_util::ends_with(value, "GiB"))
			mul *= 1024.0;

		try
		{
			int64_t iValue = static_cast<int64_t>(std::stod(value) * mul);
			if (bGPU)
				pThis->usedVRAM.fetch_add(iValue);
			else if (bCPU)
				pThis->usedRAM.fetch_add(iValue);
		}
		catch (...)
		{
			// Do nothing
		}
	}

	DebugPrint(text);
}

static LoadModelProgressCallback __LoadModelProgressCallback = nullptr; //! @thread-safety

static void OnLoadModelProgress(float progress, void* user_data)
{
	if (__LoadModelProgressCallback)
		__LoadModelProgressCallback(static_cast<int>(progress * 100.0f));
}

bool LLMInstance::InitializeChat(LLMChatArguments args)
{
	std::scoped_lock lock(_stateMutex);
	if (_readyState < ReadyState::ModelLoaded || !_modelState.pModel)
		return false;

	PushSignal(LLMStatusSignal::InitializingChat);

	_contextState = {};
	_contextState.pCtx = _modelState.pCtx;
	_contextState.pVocab = _modelState.pVocab;

	_session = args.session;

	_state = {};
#if _DEBUG
	_state.SetValue("Location", "Kitchen"); //! @temp
	_state.SetValue(_session.ApplyNames("{{char}}'s mood", Role::Bot1), "Neutral"); //! @temp
#endif

	_narratorCooldownDuration = args.narrationCooldownDuration;
	_narratorCooldown = -1;

	// Initialize sampler + grammar
	const llama_vocab* pVocab = llama_model_get_vocab(_modelState.pModel);

	// Init sampler chain
	if (_modelState.pSampler == nullptr)
	{
		llama_sampler_chain_params sampler_params = llama_sampler_chain_default_params();
		llama_sampler* pSampler = llama_sampler_chain_init(sampler_params);

		// Load grammar(s)
		string grammar = ReadTextFile("./resources/grammar/formatting_grammar.gbnf").value_or("");

		// Names
		string namesPattern;
		int32_t botCount = (int32_t)_session.GetBotCount();
		for (int i = 0; i < botCount; ++i)
		{
			if (i > 0)
				namesPattern += "| ";
			if (CheckEnumFlag(_options, LLMOption::UseCharacterIds))
				namesPattern += std::format("| \"@{}\"", _session.GetIdentifierOf(bot_from_index(i)));
			else
				namesPattern += std::format("| \"{}\"", _session.GetNameOf(bot_from_index(i)));
		}
		if (CheckEnumFlag(_options, LLMOption::AllowUserResponse))
		{
			if (CheckEnumFlag(_options, LLMOption::UseCharacterIds))
				namesPattern += std::format("| \"@{}\"", _session.GetIdentifierOf(Role::User));
			else
				namesPattern += std::format("| \"{}\"", _session.GetNameOf(Role::User));
		}
		string_util::replace_all(grammar, "##NAMES##", namesPattern);

		// Variables
		if (CheckEnumFlag(_options, LLMOption::StateVariables))
		{
			string_util::replace_all(grammar, "##STATE##", "stat");
			string_util::replace_all(grammar, "##STATE_VARS##", _state.GetGrammarPattern());
		}
		else
		{
			string_util::replace_all(grammar, "##STATE##", "");
			string_util::replace_all(grammar, "##STATE_VARS##", "[]");
		}

		llama_sampler* default_grammar_sampler = CompileGrammar(GrammarFlag::Default);

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

	// Initialize context
	llama_context* pCtx = _contextState.pCtx;
	int32_t ctx_size = llama_n_ctx(pCtx);
	llama_kv_self_clear(pCtx);
	llama_batch_free(_contextState.batch);

	llama_batch& batch = _contextState.batch = llm_util::init_batch(pCtx);
	int32_t& current_pos = _contextState.current_pos = 0;

	auto fnDecode = [&batch, pCtx](const std::vector<llama_token>& tokens, int32_t& cursor_pos) -> bool {
		int32_t n_tokens = static_cast<int32_t>(tokens.size());
		for (int i = 0; i < n_tokens; ++i)
		{
			int32_t idx = cursor_pos + i;
			batch.pos[idx] = idx;
			batch.token[idx] = tokens[i];
			batch.n_seq_id[idx] = 1;
			batch.seq_id[idx][0] = 0;
			batch.logits[idx] = false;
		}

		// Decode
		llama_batch batch_view = llm_util::create_batch_view(batch, cursor_pos, n_tokens);
		if (batch_view.n_tokens > 0 && llama_decode(pCtx, batch_view) != 0)
			return false; // Error

		batch.n_tokens += n_tokens;
		cursor_pos += n_tokens;
		return true;
	};

	string system_prompt = _session.GetSystemPrompt();
	string user_persona = _session.GetPersonaOf(Role::User);

	std::map<Role, string> personas;
	int32_t botCount = (int32_t)_session.GetBotCount();
	for (int32_t i = 0; i < botCount; ++i)
	{
		Role role = bot_from_index(i);
		personas[role] = _session.GetPersonaOf(role);
	}

	auto [template_prefix, template_suffix] = llm_tmpl::get_chat_template_prefix_suffix(Role::System, "");

	std::vector<llama_token>& system_prompt_tokens = _contextState.system_tokens = llm_util::tokenize(_modelState.pModel, template_prefix + system_prompt, false); // <BOS>?;
	_contextState.persona_pos = toI(system_prompt_tokens.size());

	// Tokenize persona(s)
	for (const auto& kvp : personas)
	{
		if (string_util::empty_or_whitespace(kvp.second))
			continue;
		
		_contextState.personas[kvp.first] = llm_util::tokenize(_modelState.pModel, kvp.second, false);

		if (!_session.IsGroupChat() || !CheckEnumFlag(_options, LLMOption::SwapPersonas))
			ContainerAppend(system_prompt_tokens, _contextState.personas[Role::Bot1]);
	}

	// User persona
	if (!string_util::empty_or_whitespace(user_persona))
	{
		auto user_persona_tokens = llm_util::tokenize(_modelState.pModel, user_persona);
		ContainerAppend(system_prompt_tokens, user_persona_tokens);
	}

	if (!fnDecode(system_prompt_tokens, current_pos))
		return false;

	// Suffix
	auto template_suffix_tokens = llm_util::tokenize(_modelState.pModel, template_suffix);
	ContainerAppend(system_prompt_tokens, template_suffix_tokens);
	if (!fnDecode(template_suffix_tokens, current_pos))
		return false;

	_contextState.blocks_pos = current_pos;

	// Initialize rng
#if _DEBUG
	_modelState.rng.seed(DEBUG_SEED);
#else
	_modelState.rng.seed((uint32_t)std::chrono::steady_clock::now().time_since_epoch().count());
#endif

	_contextState.system_tokens = system_prompt_tokens;

	_bCtxReallocateNextTurn = false;
	_readyState.store(ReadyState::Ready);
	PushSignal(LLMStatusSignal::InitializedChat);
	return true;
}

bool LLMInstance::ResetChat(int seed)
{
	if (!IsReady())
		return false;

	if (_readyState.load() >= ReadyState::Generating)
		Halt(); // Cancel ongoing generation

	{	// Lock state
		std::scoped_lock lock(_stateMutex, _resultMutex);
		ClearQueue(_resultQueue);

		if (CheckEnumFlag(_options, LLMOption::SwapPersonas))
			ActivatePersona(Role::Undefined);

		_contextState.blocks.clear();
		llama_kv_self_seq_rm(_modelState.pCtx, 0, _contextState.blocks_pos, -1);

		int32_t& current_pos = _contextState.current_pos = _contextState.blocks_pos;
		_contextState.prepend_pos = current_pos;
		_contextState.response_pos = current_pos;
		_contextState.batch.n_tokens = current_pos;

		_readyState.store(ReadyState::Ready);
	}

	if (seed > 0)
		Reseed(seed);

	PushSignal(LLMStatusSignal::InitializedChat);

	if (CheckEnumFlag(_options, LLMOption::GreetUser))
		GreetUser();
	return true;
}

void LLMInstance::__LoadModel(string filename, __LoadModelCallback onComplete)
{
	const int ngl = 99; // All layers
	usedVRAM.store(0);
	usedRAM.store(0);

	llama_backend_init();

	llama_log_set(OnLlamaLog, (void*)this);

	// initialize the model
	llama_model_params model_params = llama_model_default_params();
	model_params.n_gpu_layers = ngl;
	model_params.use_mmap = true;
	model_params.use_mlock = true;
	model_params.progress_callback = (llama_progress_callback)&OnLoadModelProgress;

	ModelState state;
	state.pModel = llama_model_load_from_file(filename.c_str(), model_params);
	state.pVocab = llama_model_get_vocab(state.pModel);
	state.modelName = string_util::get_filename(filename);

	llm_tmpl::auto_detect_template(state.pModel);

	if (!state.pModel)
	{
		fprintf(stderr, "%s: error: unable to load model\n", __func__);
		onComplete(state);
		return;
	}

	int32_t n_ctx = std::min(Constants::Context::Size, llama_model_n_ctx_train(state.pModel));

	// initialize the context
	llama_context_params ctx_params = llama_context_default_params();
	ctx_params.n_ctx = n_ctx;
	ctx_params.n_batch = n_ctx;
	ctx_params.n_ubatch = Constants::Context::MicroBatchSize;
	ctx_params.n_seq_max = 1;

	state.pCtx = llama_init_from_model(state.pModel, ctx_params);
	if (!state.pCtx)
	{
		fprintf(stderr, "%s: error: failed to create the llama_context\n", __func__);
		onComplete(state);
		return;
	}

	// Initialize embedder
	if (CheckEnumFlag(_options, LLMOption::Embeddings))
	{
		_pEmbedding = std::make_unique<LLMEmbedding>();
		if (_pEmbedding->LoadModel(string(Constants::Embedding::DefaultModelLocation)))
			DebugPrintLn("Loaded embedding model");
		else
			_pEmbedding = nullptr; // Destroy
	}

	onComplete(state);
}

bool LLMInstance::Initialize(string filename, LLMOption options, LoadModelProgressCallback onProgress, LoadModelCallback onComplete)
{
	auto readyState = _readyState.load();
	if (readyState > ReadyState::Uninitialized)
		return false; // Already loading or loaded

	_readyState.store(ReadyState::LoadingModel);
	__LoadModelProgressCallback = onProgress;
	_options = options;

	_workerThread = std::make_unique<std::jthread>(std::jthread(std::bind_front(&LLMInstance::__LoadModel, this), filename,
		[this, filename, onComplete](ModelState result)
	{
		if (result.pModel) // Success
		{
			{	// Write state
				std::scoped_lock lock(_stateMutex);
				_modelState = result;
			}
			_readyState.store(ReadyState::ModelLoaded);

			DebugPrintLn("Loaded model OK");
			PushSignal(LLMStatusSignal::LoadedModel);
			onComplete(true);
		}
		else // Failure
		{
			result.Release();

			DebugPrintLn("Failed to load model");
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
	return _readyState.load() >= ReadyState::Generating;
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
		std::scoped_lock lock(_stateMutex);

		if (_contextState.blocks.size() == 0)
			return false;

		ContextBlock& currBlock = _contextState.blocks[_contextState.blocks.size() - 1];
		if (currBlock.responseId != responseId)
			return false; // Not last message

		auto [msgType, bComplete] = llm_util::detect_message_type(currBlock.content);
		if (msgType == MessageType::Undefined || (bComplete && !extend))
			return false; // Not incomplete message

		ContextBlock block = currBlock;
		block.flags = (block.flags & ~ContextBlockFlag::Cached);
		impl_RemoveMessages(1, false); // Remove the last message, resets current_pos

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
		_contextState.response_pos = _contextState.current_pos; // Beginning of continued message
		
		generateArgs = GenerateArguments {
			/*role*/ block.role,
			/*msgType*/ msgType,
			/*flags*/ GenerateFlag::Continuation,
			/*maxMessageCount*/ 1,
			/*prepend*/ {},
			/*responseId*/ responseId,
			/*subMessageId*/ subMessageId,
		};
	} // Release state lock

	PrepareArguments prepareArgs {
		/*responder */ Role::Undefined,
		/*continue*/ true,
		/*time*/ 0,
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
		_workerThread.reset(nullptr);
	}
	_readyState.store(ReadyState::Ready);
	return true;
}

void LLMInstance::PrepareGeneration(PrepareArguments args)
{
	// Load state
	std::scoped_lock lock(_stateMutex);

	ModelState& state = _modelState;
	const llama_vocab* pVocab = llama_model_get_vocab(state.pModel);
	ContextState& ctxState = _contextState;
	llama_context* pCtx = _contextState.pCtx;
	llama_batch& batch = ctxState.batch;
	auto& blocks = ctxState.blocks;

	// Prepare prompt
	int32_t& current_pos = ctxState.current_pos;

	std::vector<llama_token> prompt_tokens;

	// Remove volatile blocks
	std::erase_if(blocks, [](const ContextBlock& block) { return block.is_volatile(); });

	// Decrement ttl
	for (int32_t i = (int32_t)blocks.size() - 1; i >= 0; --i)
	{
		auto& block = blocks[i];
		if (block.ttl <= 0)
			continue;

		block.ttl -= args.time;
		if (block.ttl > 0)
			continue;
		
		if (block.is_cached())
		{
			// Remove from context
			int32_t shift = llm_util::ctx_remove_and_shift(state.pModel, _contextState,
				blocks.begin() + (ptrdiff_t)i,
				blocks.begin() + (ptrdiff_t)(toSZ(i + 1)));

			// Adjust block offsets
			for (int32_t j = i + 1; j < (int32_t)blocks.size(); ++j)
				blocks[j].offset += shift;
			current_pos += shift;
		}

		// Remove block
		blocks.erase(blocks.begin() + (ptrdiff_t)i);
	}

	// Decrement narrator cooldown
	_narratorCooldown = std::max(_narratorCooldown - args.time, 0);

	// Tokenize uncached messages
	int32_t offset = 0;
	for (auto it = blocks.begin(); it != blocks.end(); ++it)
	{
		auto& block = *it;
		if (block.is_cached())
		{
			offset += toI(block.tokens.size());
			continue;
		}

		string content = block.content;
		if (args.isContinuation) // Continue response
			content = llm_tmpl::apply_chat_template_prefix(block.role, content, block.name); //! name?
		else if (block.role == Role::System)
			content = llm_tmpl::apply_chat_template({ Message { block.role, content, block.name } }, false);
		else
		{
			llm_util::complete_message(content);
			content = llm_tmpl::apply_chat_template({ Message { block.role, content, block.name } }, false);
		}
		content = _session.ApplyNames(content, args.responder);
		
		auto block_tokens = llm_util::tokenize(state.pModel, content, false);
		block.tokens = block_tokens;
		block.offset = offset;

		offset += toI(block.tokens.size());
		prompt_tokens.insert(prompt_tokens.end(), block_tokens.cbegin(), block_tokens.cend()); //! @assumes contiguous
	}

	// Store response position (before assistant prelude)
	if (!args.isContinuation)
		ctxState.response_pos = current_pos + (int32_t)prompt_tokens.size();

	// Calculate block positions
	ctxState.AssignBlockPositions();

	// Add state block (preface)
	if (!args.isContinuation && CheckEnumFlag(_options, LLMOption::StateVariables) && !_state.IsEmpty())
	{
		auto itUserRev = std::find_if(blocks.rbegin(), blocks.rend(), [](const ContextBlock& block) { return block.role == Role::User && !block.is_cached(); });
		auto itState = flip_iterator<ContextBlock>(blocks, itUserRev);

		itState = std::max(itState, blocks.size() > 1 ? std::max(blocks.end() - 2, blocks.begin()) : blocks.end());

		string content;
			//= string("# Story parameters\n")
			//+ "The following parameters track the state of the story and world.\n";
		content += std::format("{}", _state.GetList());
		content += "\nImportant: When events demands a parameter change, end your response with a compiled list of suggested changes.";
		content += "\nEx: <change>Param = New value</change>\n";
		content = llm_tmpl::apply_chat_template({ Message { Role::System, content } }, false);
		auto state_tokens = llm_util::tokenize(state.pModel, content, false);
		auto it = blocks.insert(itState, ContextBlock {
			/*responseId*/ "",
			/*role*/ Role::System,
			/*name*/ "",
			/*content*/ content,
			/*tokens*/ state_tokens,
			/*cached*/ ContextBlockFlag::Volatile,
		});
		ctxState.AssignBlockPositions();
		prompt_tokens.insert(prompt_tokens.begin() + ptrdiff_t((ctxState.blocks_pos + it->offset) - current_pos), state_tokens.cbegin(), state_tokens.cend());
		ctxState.response_pos = ctxState.blocks_pos + it->offset;
	}

	// Allocate and shift context window
	int n_ctx_used = llama_kv_self_used_cells(state.pCtx);
	size_t ctx_reserve = std::max((int32_t)prompt_tokens.size() + Constants::Context::MaxResponseLength, Constants::Context::MicroBatchSize);
	size_t ctx_size = llama_n_ctx(pCtx);
	if (n_ctx_used + ctx_reserve >= ctx_size || _bCtxReallocateNextTurn)
	{
		DebugPrintLn(">> ALLOCATING CONTEXT");
		_bCtxReallocateNextTurn = false;

		size_t ctx_chat_max = ctx_size - ctxState.blocks_pos; // Exclude system prompt
		size_t free_tokens = std::max(static_cast<int32_t>(ctx_reserve), static_cast<int32_t>(ctx_chat_max * (1.0f - Constants::Context::WindowKeepRatio)));
		
		size_t total = 0;
		size_t first_to_keep = 0;
		while (first_to_keep < blocks.size() && total < free_tokens && blocks[first_to_keep].is_cached())
			total += blocks[first_to_keep++].length();

		if (first_to_keep > 0)
		{
			auto itBegin = blocks.begin();
			auto itEnd = blocks.begin() + (ptrdiff_t)first_to_keep;
			int32_t shift = llm_util::ctx_remove_and_shift(state.pModel, ctxState, itBegin, itEnd);

			// Adjust block offsets
			for (auto& block : blocks)
				block.offset += shift;

			blocks.erase(itBegin, itEnd);
			current_pos += shift;
		}
	}

	// Append assistant tokens
	if (!args.isContinuation)
	{
		auto [prelude, _] = llm_tmpl::get_chat_template_prefix_suffix(args.responder, "assistant"); //! @name?
		prelude = _session.ApplyNames(prelude, args.responder);
		auto assistant_tokens = llm_util::tokenize(state.pModel, prelude, false);
		prompt_tokens.insert(prompt_tokens.end(), assistant_tokens.begin(), assistant_tokens.end());
	}

	// Store beginning of response (after assistant prelude)
	ctxState.prepend_pos = current_pos + (int32_t)prompt_tokens.size();

	// Append to batch
	for (int i = 0; i < prompt_tokens.size(); ++i)
		common_batch_add(batch, prompt_tokens[i], current_pos + i, { 0 }, false);
	batch.n_tokens = current_pos + (int32_t)prompt_tokens.size();

	// Mark blocks in cache
	for (auto it = blocks.begin(); it != blocks.end(); ++it)
		it->flags = (it->flags | ContextBlockFlag::Cached);

	llm_util::dump_context(batch, ctxState.pVocab, "prompt-full.txt");
}

void LLMInstance::__Generate(std::stop_token thread_stop, GenerateArguments args, __PartialResultCallback onPartial, __GenerationCompleteCallback onComplete)
{
	std::unique_lock<std::timed_mutex> stateLock(_stateMutex, std::defer_lock);
	if (!stateLock.try_lock_for(100ms))
	{
		onComplete(InternalError::UnknownError, "Failed to acquire lock");
		return;
	}

	std::vector<llama_token> sampled_tokens;
	ModelState& state = _modelState;
	const llama_vocab* pVocab = llama_model_get_vocab(state.pModel);

	llama_token sampled_token;
	string partial;
	string stop_reason;
	string response;
	MessageType msgType = args.msgType;
	bool isContinuation = CheckEnumFlag(args.flags, GenerateFlag::Continuation);
	bool isInstigation = CheckEnumFlag(args.flags, GenerateFlag::Instigation);

	ContextState& chat = _contextState;
	llama_batch& batch = chat.batch;
	int32_t n_batch = llama_n_batch(state.pCtx);
	int32_t ctx_size  = llama_n_ctx(state.pCtx);
	int32_t& current_pos = chat.current_pos;
	string userName = _session.GetNameOf(Role::User);
	int32_t& pre_response_pos = chat.response_pos;

	string responseId = args.responseId.empty() ? CreateUUID() : args.responseId;
	string subMessageId = args.subMessageId.empty() ? CreateUUID() : args.subMessageId;

	int numMessages = 0;
	Role responderRole = args.role;
	string responderId {};
	string stateReport {};

	assert(llama_kv_self_used_cells(state.pCtx) + Constants::Context::MaxResponseLength <= ctx_size);

	if (!args.history.empty() && _pEmbedding)
	{
		DebugPrintLn(">> SEARCH EMBEDDINGS");
		_pEmbedding->Search(args.history, true, true);
	}

	DebugPrintLn(">> BEGIN GENERATION");

	// Select and init grammar
	GrammarFlag grammarFlags = GrammarFlag::None;
	if (args.msgType != MessageType::Undefined)
	{
		if (isContinuation)
			grammarFlags = grammarFlags | GrammarFlag::Continue;
		else if (isInstigation)
			grammarFlags = grammarFlags | GrammarFlag::Stub;
		else
			grammarFlags = grammarFlags | GrammarFlag::Default;

		if (args.msgType == MessageType::Dialogue)
			grammarFlags = grammarFlags | GrammarFlag::Talk;
		else if (args.msgType == MessageType::Action)
			grammarFlags = grammarFlags | GrammarFlag::Act;
		else if (args.msgType == MessageType::Narration)
			grammarFlags = grammarFlags | GrammarFlag::Narrate;
	}
	else
		grammarFlags = GrammarFlag::Default;

	if (CheckEnumFlag(args.flags, GenerateFlag::AllowNarrator))
		grammarFlags = grammarFlags | GrammarFlag::EnableNarrator;
	if (CheckEnumFlag(_options, LLMOption::StateVariables))
		grammarFlags = grammarFlags | GrammarFlag::EnableState;

	CompileGrammar(grammarFlags);
	if (auto pGrammar = state.SetActiveGrammar(grammarFlags))
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

	auto startTime = std::chrono::steady_clock::now();

	while (true)
	{
		bool next_token = true;

		if (thread_stop.stop_requested())
		{
			stop_reason = "cancel";
			break; // Cancelled
		}

		if (current_pos >= ctx_size)
		{
			stop_reason = "max ctx";
			break; // Max limit reached
		}

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
			onComplete(InternalError::ContextFull, std::format("context size exceeded ({} / {})", n_ctx_used, ctx_size));
			return;
		}
		
		if (batch_view.n_tokens > 0)
		{
			int r = llama_decode(state.pCtx, batch_view);
			if (r == 0) // Success
			{
				current_pos += batch_view.n_tokens;
			}
			else if (r < 0) // Error
			{
				onComplete(InternalError::DecodeError, "llama_decode returned error");
				return;
			}
			else if (r > 0) // Insufficient cache size (possibly due to fragmentation)
			{
#if FALSE
				DebugPrintLn(">> DEFRAGMENTING");
				llama_kv_self_defrag(state.pCtx);
				llama_kv_self_seq_rm(state.pCtx, 0, current_pos, -1);
				batch.n_tokens = current_pos;

				if (llama_decode(state.pCtx, batch_view))
				{
					onComplete(InternalError::DecodeError, "llama_decode returned error");
					return;
				}
#else
				DebugPrintLn(">> REBUILDING CONTEXT");
				if (!RebuildKVCache(state.pCtx, batch))
				{
					onComplete(InternalError::DecodeError, "llama_decode returned error");
					return;
				}
				_bCtxReallocateNextTurn = true; // Also free up more memory
#endif
			}
		}


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
			llm_util::process(partial, str_token, &bWait, &bHalt, stop_reason);
		}
		else // EOG token
		{
			bHalt = true;
			stop_reason = "EOG";
		}

		if (sampled_tokens.size() >= Constants::Context::MaxResponseLength)
		{
			bHalt = true;
			stop_reason = "length";
		}

		bSend &= !bWait;
		next_token &= !bHalt;

		// Send/Queue result
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

					if (tagName == "@USR" || tagName == _session.GetNameOf(Role::User) && args.role != Role::User)
					{
						stop_reason = "user";
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
						if (fmt_start > 0) // Remainder: Send it first
						{
							carryOver = partial.substr(fmt_start);
							partial.erase(fmt_start);
							sendMsg = !partial.empty();
							responderId = llm_util::format_id(tagName);
							responderRole = _session.GetRoleOf(responderId);
						}
						else // No remainder: New message
						{
							sendMsg.erase(fmt_start, fmt_end - fmt_start + 1);
							responderId = llm_util::format_id(tagName);
							responderRole = _session.GetRoleOf(responderId);

							if (tag == Constants::Chat::DialogueTag)
								msgType = MessageType::Dialogue;
							else if (tag == Constants::Chat::ActionTag)
								msgType = MessageType::Action;
							else if (tag == Constants::Chat::ThoughtTag)
								msgType = MessageType::Thought;
							else if (tag == Constants::Chat::NarrationTag)
								msgType = MessageType::Narration;
							else if (tag == Constants::Chat::DirectionTag)
								msgType = MessageType::Direction;
							else if (tag == Constants::Chat::StateReportTag)
								msgType = MessageType::StateReport;

							if (msgType != MessageType::StateReport && args.maxMessages > 0 && ++numMessages >= args.maxMessages)
							{
								stop_reason = "msg count";
								break; // That's enough, thank you
							}

							if (msgType == MessageType::Narration)
								_narratorCooldown = _narratorCooldownDuration;

							if (CheckEnumFlag(_options, LLMOption::SwapPersonas))
								ActivatePersona(responderRole);
						}
					}
				}
				else
				{
					// Wait for more
					carryOver = partial + carryOver;
					partial.clear();
				}
			}

//			if (msgType == MessageType::Undefined)
//				msgType = MessageType::Dialogue;

			// Send piece
			if (partial.size() > 0 && msgType == MessageType::StateReport)
			{
				stateReport += partial;
			}
			else if (partial.size() > 0 && msgType != MessageType::Undefined)
			{
				std::scoped_lock lock(_resultMutex);

				_resultQueue.push(MessagePiece {
					/*responseId*/ responseId,
					/*subMessageId*/ subMessageId,
					/*identifier*/ responderId,
					/*text*/ sendMsg,
					/*role*/ responderRole,
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
				subMessageId = CreateUUID();
				PushSignal(LLMStatusSignal::CompletedMessage);
			}
		}

		// Print to console
		printf("%s", str_token.c_str());
		assert(current_pos < ctx_size);

		// Add sampled token to batch
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

	// Remove full response from cache (will be reinserted on next turn)
	llama_kv_self_seq_rm(state.pCtx, 0, pre_response_pos, current_pos);
	batch.n_tokens = pre_response_pos;
	current_pos = pre_response_pos;

	for (auto& block : chat.blocks)
	{
		if (chat.blocks_pos + block.offset >= pre_response_pos)
			block.flags = (block.flags & ~ContextBlockFlag::Cached);
	}

	_state.UpdateValues(stateReport);

	llm_util::sanitize_response(response);

	if (response.length() > 0)
	{
		if (isContinuation)
		{
			auto& lastBlock = chat.blocks[chat.blocks.size() - 1];
			lastBlock.content += response;
			lastBlock.tokens.insert(lastBlock.tokens.end(), sampled_tokens.begin(), sampled_tokens.end());
		}
		else
		{
			chat.blocks.push_back(ContextBlock {
				/*responseId*/ responseId,
				/*role*/ responderRole,
				/*name*/ responderId,
				/*content*/ response,
				/*tokens*/ sampled_tokens,
				/*flags*/ ContextBlockFlag::None,
				/*offset*/ pre_response_pos - chat.blocks_pos,
			});
		}
	}
	else
	{
		DebugPrint("(Empty response)");
	}

	DebugPrintLn();
	DebugPrintLn(std::format("END OF GENERATION (stopped on:{}) [{}]", stop_reason.c_str(), sampled_tokens.size()));

	onComplete(InternalError::NoError, response);
};

void LLMInstance::StartGeneration(GenerateArguments args)
{
	if (CheckEnumFlag(_options, LLMOption::RandomizeMessageCount))
	{	// Acquire state lock
		std::scoped_lock lock(_stateMutex);
		static std::uniform_int_distribution<int> numMessages(1, 3);
		if (args.maxMessages <= 0) // Randomize number of messages
			args.maxMessages = numMessages(_modelState.rng);
	}
	else if (!CheckEnumFlag(_options, LLMOption::LimitMessages))
		args.maxMessages = 0;

	if (args.responseId.empty())
		args.responseId = CreateUUID();
	if (args.subMessageId.empty())
		args.subMessageId = CreateUUID();

	{
		std::scoped_lock(_resultMutex);
		_activeResponseIds.insert(args.responseId);
	}

	_readyState.store(ReadyState::Generating);
	PushSignal(LLMStatusSignal::GenerationStarted);

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
		/*responder */ Role::Undefined,
		/*continue*/ false,
		/*time*/ 1,
	};
	PrepareGeneration(prepareArgs);

	GenerateFlag flags = GenerateFlag::None;
	if (_narratorCooldown <= 0)
		flags = flags | GenerateFlag::AllowNarrator;

	GenerateArguments generateArgs {
		/*role*/ Role::Bot1,	 // @role
		/*msgType*/ MessageType::Undefined,
		/*flags*/ flags,
	};
	generateArgs.history = GetHistory(Constants::Embedding::Depth);

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
	string identifier = CheckEnumFlag(_options, LLMOption::UseCharacterIds) ? "@" +_session.GetIdentifierOf(role) : _session.GetNameOf(role);
	string content = message;
	content = _session.ApplyNames(content);
	std::vector<Submessage> subMessages;
	content = llm_util::process_message(content, identifier, &subMessages);

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
			/*name*/ identifier,
			/*content*/ content,
			/*tokens*/ {},
			/*cached*/ ContextBlockFlag::None,
			/*offset*/ 0,
			/*ttl*/ ttl > 0 ? ttl + 1 : 0,
		});
	}

	if (visible)
	{
		std::scoped_lock lock(_resultMutex);
		_activeResponseIds.insert(responseId);
		
		// Add message to result queue
		for (auto subMsg : subMessages)
		{
			string subMessageId = CreateUUID();
			_resultQueue.push(MessagePiece {
				/*blockId*/ responseId,
				/*messageId*/ subMessageId,
				/*identifier*/ identifier,
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

		if (_narratorCooldown > 0)
			_narratorCooldown = std::min(_narratorCooldown + toI(numRemovals), _narratorCooldownDuration);
	}

	if (newSize > 0)
	{
		auto& newLastblock = _contextState.blocks[newSize - 1_sz];
		if (newLastblock.is_cached())
			current_pos = std::min(current_pos, _contextState.blocks_pos + newLastblock.offset + newLastblock.length());
		else
			current_pos = std::min(current_pos, _contextState.blocks_pos + newLastblock.offset);
	}
	else
	{
		current_pos = _contextState.blocks_pos;
	}
	
	// Update batch
	_contextState.batch.n_tokens = current_pos;

	// Clear kv cache
	llama_kv_self_seq_rm(_contextState.pCtx, 0, current_pos, -1);

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

	if (auto text = ReadTextFile("./resources/prompting/prompt_greeting.txt"))
	{
		string greetingInstruction = _session.ApplyNames(text.value());
		PushMessage(Role::Director, "{{" + greetingInstruction + "}}", MessageType::Direction, false, 1);
		InstigateResponse(Role::Undefined, MessageType::Dialogue, 3);
	}
	return true;
}

bool LLMInstance::Instruct(string instructions)
{
	if (!CanGenerate())
		return false;

	if (auto text = ReadTextFile("./resources/prompting/prompt_formatting_director.txt"))
	{
		string prompt = text.value();
		prompt = _session.ApplyNames(prompt);
		PushMessage(Role::System, prompt, MessageType::SystemMessage, false, 1);
	}

	PushMessage(Role::Director, "{{" + instructions + "}}", MessageType::Direction, false, 4);
	InstigateResponse(Role::Undefined, MessageType::Undefined, 3);
	return true;
}

bool LLMInstance::InstigateResponse(Role role, MessageType msgType, int messageCount)
{
	if (!CanGenerate())
		return false;
	
	if (role == Role::Undefined)
		role = Role::Bot1;

	PrepareArguments prepareArgs {
		/*responder */ role,
		/*continue*/ false,
		/*time*/ 1,
	};
	PrepareGeneration(prepareArgs);

	string responder = CheckEnumFlag(_options, LLMOption::UseCharacterIds) ? "@" + _session.GetIdentifierOf(role) : _session.GetNameOf(role);

	bool bAllowNarration = true;

	string prependMsg;
	if (msgType == MessageType::Dialogue)
	{
		prependMsg = std::format("<{}=\"{}\">", Constants::Chat::DialogueTag, responder);
		bAllowNarration = false;
	}
	else if (msgType == MessageType::Action)
	{
		prependMsg = std::format("<{}=\"{}\">", Constants::Chat::ActionTag, responder);
		bAllowNarration = false;
	}
	else if (msgType == MessageType::Thought)
	{
		prependMsg = std::format("<{}=\"{}\">", Constants::Chat::ThoughtTag, responder);
		bAllowNarration = false;
	}
	else if (msgType == MessageType::Narration)
	{
		prependMsg = std::format("<{}>", Constants::Chat::NarrationTag);
		_narratorCooldown = _narratorCooldownDuration;
	}
	else if (msgType == MessageType::Direction)
	{
		prependMsg = std::format("<{}>", Constants::Chat::DirectionTag);
		bAllowNarration = false;
	}

	bAllowNarration &= _narratorCooldown <= 0 || msgType == MessageType::Narration;

	GenerateArguments generateArgs {
		/*role*/ role,
		/*msgType*/ msgType,
		/*flags*/ GenerateFlag::Instigation | (bAllowNarration ? GenerateFlag::AllowNarrator : GenerateFlag::None),
		/*maxMessageCount*/ messageCount,
		/*prepend*/ prependMsg,
	};
	generateArgs.history = GetHistory(Constants::Embedding::Depth);
	
	StartGeneration(generateArgs);
	return true;
}

std::pair<LLMStatus, bool> LLMInstance::PollStatus()
{
	LLMStatus status {};

	// Try to acquire state lock
	std::unique_lock<std::timed_mutex> lock(_stateMutex, std::try_to_lock);
	if (lock.owns_lock())
	{
		if (_modelState.pModel && _modelState.pCtx)
		{
			status.modelName = _modelState.modelName;
			status.allocCtxSize = llama_n_ctx(_modelState.pCtx);
			status.usedCtxSize = llama_kv_self_used_cells(_modelState.pCtx);
			status.tokensPerSec = _tokensPerSec.load();
			status.usedVRAM = usedVRAM.load();
			status.usedRAM = usedRAM.load();
		}
		lock.unlock();
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

#if _DEBUG
bool LLMInstance::DumpContext(bool full, string filename) const
{
	const ModelState& state = _modelState;
	const llama_vocab* pVocab = llama_model_get_vocab(state.pModel);
	const llama_batch& batch = _contextState.batch;

	auto fnTokenStr = [pVocab, full](llama_token token, bool quote) -> string {
		if (token <= 0)
			return "<UNK>";
		else if (token == llama_vocab_bos(pVocab))
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
			return full ? "<NL>" : "\r\n";
		else
		{
			char buf[256];
			int n = llama_token_to_piece(pVocab, token, buf, sizeof(buf), 0, true);
			if (n < 0)
				return "<UNK>";
			else
				return quote ? "\"" + string(buf, n) + "\"" : string(buf, n);
		}
	};

	if (batch.token == nullptr || batch.n_tokens == 0)
		return false;

	// Detokenize the batched tokens
	string result;
	result.reserve(65536);
	int32_t size = full ? Constants::Context::Size : batch.n_tokens;
	if (full)
	{
		for (int32_t i = 0; i < size; ++i)
			result.append(std::format("{0:<8}{1:<8}{2}\r\n", batch.pos[i], batch.token[i], fnTokenStr(batch.token[i], true)));
	}
	else
	{
		for (int32_t i = 0; i < size; ++i)
			result.append(fnTokenStr(batch.token[i], false));
	}

	if (!full)
		result.append(std::format("[pos:{0}/{1}]\r\n", _contextState.current_pos, batch.n_tokens));

	// Cached blocks
	for (auto& block : _contextState.blocks)
	{
		if (block.is_cached())
			continue;

		result.append("[");
		if (!block.tokens.empty())
		{
			for (int32_t i = 0; i < block.length(); ++i)
				result.append(fnTokenStr(block.tokens[i], false));
		}
		else
		{
			result.append(block.content);
		}
		result.append("]\r\n");
	}

	return WriteTextFile(filename, result, false);
}
#endif 

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
	return std::set<string>(_activeResponseIds.begin(), _activeResponseIds.end()); // Copy
}

void LLMInstance::RefreshActiveResponses()
{
	std::scoped_lock lock(_resultMutex);
	_activeResponseIds.clear();
	for (auto it = _contextState.blocks.cbegin(); it != _contextState.blocks.cend(); ++it)
		_activeResponseIds.insert(it->responseId);
}

bool LLMInstance::ActivatePersona(Role role)
{
	if (!_session.IsGroupChat() || !(is_bot(role) || role == Role::Undefined))
		return false;

	if (role == _contextState.activePersona)
		return false; // No change

	llama_batch& batch = _contextState.batch;
	llama_context* pCtx = _modelState.pCtx;

	// Remove current persona
	auto itFindActive = _contextState.personas.find(_contextState.activePersona);
	if (itFindActive != _contextState.personas.end())
	{
		auto& tokens = itFindActive->second;
		int32_t len = toI(tokens.size());

		// Remove from kv cache
		int32_t shift = llm_util::batch_remove(pCtx, batch, _contextState.persona_pos, _contextState.persona_pos + len);

		_contextState.current_pos -= shift;
		_contextState.blocks_pos -= shift;
		_contextState.response_pos -= shift;
		_contextState.prepend_pos -= shift;

		_contextState.activePersona = Role::Undefined;
	}

	// Activate next persona
	auto itFindInactive = _contextState.personas.find(role);
	if (itFindInactive != _contextState.personas.end())
	{
		auto& tokens = itFindInactive->second;
		int32_t len = toI(tokens.size());

		//! TODO: Allocate enough space

		// Shift down
		int32_t shift = llm_util::batch_allocate(pCtx, batch, _contextState.persona_pos, len);
		
		// Write persona to batch
		llm_util::batch_write(_modelState.pModel, pCtx, batch, tokens, _contextState.persona_pos);

		// Decode
		llama_batch batch_view = llm_util::create_batch_view(_contextState.batch, _contextState.persona_pos, len);
		if (batch_view.n_tokens > 0 && llama_decode(_modelState.pCtx, batch_view) != 0)
			return false; // Error

		_contextState.current_pos += len;
		_contextState.blocks_pos += len;
		_contextState.response_pos += len;
		_contextState.prepend_pos += len;

		_contextState.activePersona = role;
		return true;
	}

	return false;
}

Sentences LLMInstance::GetHistory(size_t depth)
{
	std::scoped_lock lock(_stateMutex);
	Sentences sentences;

	int n = 0;
	for (auto it = _contextState.blocks.crbegin(); it != _contextState.blocks.crend() && n < depth; ++it, ++n)
	{
		auto& block = *it;
		string msg = string_util::trim(block.content);

		size_t pos_begin = msg.find('>', 0);
		while (pos_begin != string::npos)
		{
			size_t pos_end = msg.find('<', pos_begin);
			if (pos_end == string::npos)
				break;

			string content = msg.substr(pos_begin + 1, pos_end - pos_begin - 1);
			if (content.length() > 2)
			{
				if (content.front() == '*' && content.back() == '*')
					content = content.substr(1, content.length() - 2);
				else if (content.front() == '"' && content.back() == '"')
					content = content.substr(1, content.length() - 2);
			}

			if (Constants::Embedding::SplitSentences)
			{
				// Split into individual sentences for RAG
				string_util::replace_all(content, "?", ".");
				string_util::replace_all(content, "!", ".");
				auto split = string_util::split(content, '.', true);
				for (auto& s : split)
					sentences.insert(sentences.begin(), { block.role, s });
			}
			else
			{
				sentences.insert(sentences.begin(), { block.role, content });
			}

			pos_begin = msg.find('>', pos_end + 1);
			if (pos_begin == string::npos)
				break;
			pos_begin = msg.find('>', pos_begin + 1);
		}
	}

	return sentences;
}

#if _DEBUG
bool LLMInstance::GenerateEmbedding(string text)
{
	if (!_pEmbedding)
		return false;

	EmbeddingVector embedding;
	if (_pEmbedding->Generate(text, embedding))
	{
		Embeddings::AddEmbedding(embedding);

		// Save to disk
		string filename = std::format("./embeddings/{}.txt", CreateUUID());
		embedding.SaveToFile(filename);
	}
	return true; // Break here
}
#endif

bool LLMInstance::RebuildKVCache(llama_context* pCtx, const llama_batch& batch)
{
	PushSignal(LLMStatusSignal::RebuildingContext);
	auto prevReadyState = _readyState.exchange(ReadyState::RebuildingContext);

#if FALSE
	llama_kv_self_clear(pCtx);
	int r = llama_decode(pCtx, batch);
#else
	llama_kv_self_seq_rm(pCtx, 0, _contextState.blocks_pos, -1);
	auto batch_view = llm_util::create_batch_view(batch, _contextState.blocks_pos, batch.n_tokens - _contextState.blocks_pos);
	int r = llama_decode(pCtx, batch_view);
#endif

	_contextState.current_pos = batch.n_tokens;

	_readyState.store(prevReadyState);
	PushSignal(LLMStatusSignal::GenerationStarted);
	return r == 0;
}

llama_sampler* LLMInstance::CompileGrammar(GrammarFlag flags)
{
	if (flags == GrammarFlag::None)
		return nullptr;

	auto itFind = _modelState.grammars.find(flags);
	if (itFind != _modelState.grammars.end())
		return itFind->second;

	DebugPrintLn(std::format("Compiling grammar variant 0x{:X}", (int32_t)flags));
	llama_sampler* pGrammar = llm_util::compile_grammar(
		flags,
		_modelState.pVocab, 
		_session.GetNameGrammar(CheckEnumFlag(_options, LLMOption::UseCharacterIds), CheckEnumFlag(_options, LLMOption::AllowUserResponse)), 
		_state.GetGrammarPattern());
	
	_modelState.grammars[flags] = pGrammar;
	return pGrammar;
}