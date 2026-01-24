#include <pch.h>
#include "llm/LLMInstance.h"
#include "llm/LLMUtility.h"
#include "llm/LLamaApi.h"
#include "llm/LLMTemplate.h"
#include "llm/LLMStatus.h"
#include "llm/Embedding.h"
#include "util/StringUtility.h"
#include "util/FileUtility.h"
#include "util/Common.h"
#include "util/Lockable.h"
#include <format>
#include <algorithm>
#include <cassert>
#include <chrono>

using namespace std::chrono_literals;
using namespace fig::common_util;
using namespace fig::fs;
using namespace fig::string_util;
using namespace fig::llm;
using namespace fig::data;

template<typename T>
void queue_clear(std::queue<T>& q)
{
	std::queue<T> empty;
	std::swap(q, empty);
}

inline constexpr fig::string Direction(fig::string_view text)
{
	return "{{" + toStr(text) + "}}";
}

inline constexpr fig::string Narration(fig::string_view text)
{
	return "[" + toStr(text) + "]";
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
}

bool LLMInstance::Initialize(LLMChatArguments args)
{
	_pStatus->EmitSignal(LLMStatusSignal::ChatInitializing);

	bool bMultiSequence = args.session.IsGroupChat() && args.options.groupChatMode == ChatOptions::GroupChatMode::SwapSequences;
	int32_t n_bots = bMultiSequence ? (int32_t)args.session.GetBotCount() : 1;
	if (n_bots == 0)
		return false; // Error

	std::scoped_lock _(_stateMutex); // Lock for the entire duration of the scope

	_contextState = Context(_modelState, _modelState.max_sequences);
	_session = args.session;
	_stateVars = {};
	_turn_counter.store(0);
	_options = args.options;

	// Read personas
	std::map<Role, fig::string> personas;
	int32_t botCount = (int32_t)_session.GetBotCount();
	for (int32_t i = 0; i < botCount; ++i)
	{
		Role role = bot_from_index(i);
		personas[role] = _session.GetPersonaOf(role);
	}
	fig::string user_persona = _session.GetPersonaOf(Role::User);

	if (!personas.contains(Role::Bot1))
		return false; // No main character

	_next_narrator_turn = -1;
	_narratorCooldownDuration = args.narrationCooldownDuration;

	// Initialize sampler + grammar
	InitSamplers();

	// Initialize rng
#if _DEBUG
	_modelState.rng.seed(Constants::LLM::DebugSeed);
#else
	_modelState.rng.seed((uint32_t)std::chrono::steady_clock::now().time_since_epoch().count());
#endif

	VocabPtr pVocab = llama::get_vocab(_modelState.pModel);

	// Init state variables (temp)
	if (args.options.flags.IsSet(ChatOptions::Flag::StateVariables))
	{
		_stateVars.SetValue("Location", "Kitchen"); //! @temp
		_stateVars.SetValue(_session.ApplyNames("{{char}}'s mood", Role::Bot1), "Neutral"); //! @temp
	}

	// Initialize context
	_contextState.Initialize();
	ContextCursor& cursor_pos = _contextState.cursor_pos = 0;

	auto [template_prefix, template_suffix] = llm_tmpl::get_chat_template_prefix_suffix(Role::System, "");

	if (bMultiSequence) // Initialize a sequence for each bot
	{
		// Write (shared) system_prompt
		fig::string system_prompt = _session.GetSystemPrompt(); //! @group

		std::vector<Token> system_prompt_tokens = llama::tokenize(pVocab, template_prefix + system_prompt, false); // <BOS>?;

		// System prompt
		_contextState.AppendBlock(ContextBlock {
			.role = Role::System,
			.name = "",
			.content = template_prefix + system_prompt,
			.tokens =  system_prompt_tokens,
			.flags { ContextBlockFlag::Static },
			.attn_position = 0,
			.sequenceSlots { SequenceSlot::Shared },
		});
		
		int32_t persona_pos = toI(system_prompt_tokens.size());

		// Write persona(s)
		for (int32_t i = 0; i < n_bots; ++i)
		{
			Role role = bot_from_index(i);
			SequenceSlots seq_id = fig::llm_util::get_sequence_from_index(i);

			// Persona
			std::vector<Token> persona_tokens = llama::tokenize(pVocab, personas[role], false);
			_contextState.AppendBlock(ContextBlock {
				.role = Role::System,
				.name = "",
				.content = personas[role],
				.tokens = persona_tokens,
				.flags { ContextBlockFlag::Static,  ContextBlockFlag::Persona },
				.attn_position = persona_pos,
				.sequenceSlots = seq_id,
			});
		}

		// User persona
		if (!empty_or_whitespace(user_persona))
		{
			auto user_persona_tokens = llama::tokenize(pVocab, user_persona);
			_contextState.AppendBlock(ContextBlock {
				.role = Role::System,
				.name = "",
				.content = user_persona,
				.tokens = user_persona_tokens,
				.flags { ContextBlockFlag::Static },
				.sequenceSlots { SequenceSlot::Shared },
			});
		}

		// Template suffix
		auto template_suffix_tokens = llama::tokenize(pVocab, template_suffix);
		_contextState.AppendBlock(ContextBlock {
			.role = Role::System,
			.name = "",
			.content = template_suffix,
			.tokens = template_suffix_tokens,
			.flags { ContextBlockFlag::Static },
			.sequenceSlots { SequenceSlot::Shared },
		});
	}
	else // Single sequence
	{
		if (_session.IsGroupChat())
		{
			// Tokenize all personas
			for (const auto& kvp : personas)
				_contextState.personas[kvp.first] = llama::tokenize(pVocab, kvp.second, false);
		}

		fig::string system_prompt = _session.GetSystemPrompt();
		std::vector<Token> system_prompt_tokens = llama::tokenize(pVocab, template_prefix + system_prompt, false); // <BOS>?;

		_contextState.AppendBlock(ContextBlock {
			.role = Role::System,
			.name = "",
			.content = template_prefix + system_prompt,
			.tokens = system_prompt_tokens,
			.flags { ContextBlockFlag::Static },
			.attn_position = 0,
			.sequenceSlots { SequenceSlot::Default },
		});

		int32_t persona_pos = toI(system_prompt_tokens.size());

		std::vector<Token> persona_tokens = llama::tokenize(pVocab, personas[Role::Bot1], false);
		_contextState.AppendBlock(ContextBlock {
			.role = Role::System,
			.name = "",
			.content = personas[Role::Bot1],
			.tokens = persona_tokens,
			.flags { ContextBlockFlag::Static, ContextBlockFlag::Persona },
			.attn_position = persona_pos,
			.sequenceSlots { SequenceSlot::Default },
		});

		// User persona
		if (!empty_or_whitespace(user_persona))
		{
			auto user_persona_tokens = llama::tokenize(pVocab, user_persona);
			_contextState.AppendBlock(ContextBlock {
				.role = Role::System,
				.name = "",
				.content = user_persona,
				.tokens = user_persona_tokens,
				.flags { ContextBlockFlag::Static },
				.sequenceSlots { SequenceSlot::Default },
			});
		}

		// Template suffix
		auto template_suffix_tokens = llama::tokenize(pVocab, template_suffix);
		_contextState.AppendBlock(ContextBlock {
			.role = Role::System,
			.name = "",
			.content = template_suffix,
			.tokens = template_suffix_tokens,
			.flags { ContextBlockFlag::Static },
			.sequenceSlots { SequenceSlot::Default },
		});
	}

	_contextState.TokenizeUncached(_session);
	auto pos_attn = _contextState.DecodeUncached();
	if (not pos_attn.is_valid())
		return false; // Error

	_contextState.chat_begin_pos = pos_attn;
	_contextState.token_pos = pos_attn;
	_contextState.cursor_pos = _contextState.GetUncachedOffset();
	_contextState.active_persona = Role::Bot1;
	
	SetReadyState(ReadyState::Ready);
	_pStatus->EmitSignal(LLMStatusSignal::ChatInitialized);

	return true;
}

void LLMInstance::InitSamplers()
{
	// Init sampler chain
	if (_modelState.pSampler != nullptr)
		return;

	llama_sampler_chain_params sampler_params = llama_sampler_chain_default_params();
	SamplerPtr pSampler = llama_sampler_chain_init(sampler_params);

	// Load grammar(s)
	fig::string grammar = ReadTextFile("./resources/grammar/formatting_grammar.gbnf").value_or("");

	// Names
	fig::string namesPattern;
	int32_t botCount = (int32_t)_session.GetBotCount();
	for (int i = 0; i < botCount; ++i)
	{
		if (i > 0)
			namesPattern += "| ";
		if (_options.flags.IsSet(ChatOptions::Flag::UseCharacterIds))
			namesPattern += std::format("| \"@{}\"", _session.GetIdentifierOf(bot_from_index(i)));
		else
			namesPattern += std::format("| \"{}\"", _session.GetNameOf(bot_from_index(i)));
	}
	if (_options.flags.IsSet(ChatOptions::Flag::AllowUserResponse))
	{
		if (_options.flags.IsSet(ChatOptions::Flag::UseCharacterIds))
			namesPattern += std::format("| \"@{}\"", _session.GetIdentifierOf(Role::User));
		else
			namesPattern += std::format("| \"{}\"", _session.GetNameOf(Role::User));
	}
	replace_all_inplace(grammar, "##NAMES##", namesPattern);

	// Variables
	if (_options.flags.IsSet(ChatOptions::Flag::StateVariables))
	{
		replace_all_inplace(grammar, "##STATE##", "stat");
		replace_all_inplace(grammar, "##STATE_VARS##", _stateVars.GetGrammarPattern());
	}
	else
	{
		replace_all_inplace(grammar, "##STATE##", "");
		replace_all_inplace(grammar, "##STATE_VARS##", "[]");
	}

	uint32_t random_seed;
	if constexpr (Debugging)
		random_seed = Constants::LLM::DebugSeed;
	else
		random_seed = Constants::LLM::RandomSeed;

	auto default_grammar_sampler = CompileGrammar({ GrammarFlag::Default });

	// Grammar
	if (default_grammar_sampler) 
		llama_sampler_chain_add(pSampler, default_grammar_sampler);
	
	// Dry (Details: https://github.com/oobabooga/text-generation-webui/pull/5677)
	static auto dry_seq_breakers = std::array<const char*, 5> { "\n", ":", "\"", "*", "]" };
	llama_sampler_chain_add(pSampler, llama_sampler_init_dry(_modelState.pVocab, _modelState.ctx_size, 0.8f, 1.75f, 2, 256, dry_seq_breakers.data(), dry_seq_breakers.size()));

	// Repeat penalty (Off)
	if constexpr (Disabled)
		llama_sampler_chain_add(pSampler, llama_sampler_init_penalties(512, 1.05f, 0.0f, 0.0f));

	// Min P sampler
	llama_sampler_chain_add(pSampler, llama_sampler_init_min_p(0.05f, 0));
	
	// Temperature
	llama_sampler_chain_add(pSampler, llama_sampler_init_temp(0.9f));

	// XTC (Details: https://github.com/oobabooga/text-generation-webui/pull/6335)
	llama_sampler_chain_add(pSampler, llama_sampler_init_xtc(0.5f, 0.1f, 0, random_seed));

	// Random seed
	llama_sampler_chain_add(pSampler, llama_sampler_init_dist(random_seed));

	_modelState.pSampler = pSampler;
	_modelState.pActiveGrammar = default_grammar_sampler;
}

bool LLMInstance::ResetChat(int seed)
{
	if (!IsReady())
		return false;

	if (_readyState.load() >= ReadyState::Generating)
		Halt(); // Cancel ongoing generation

	{	// Lock state
		std::scoped_lock lock(_stateMutex, _resultMutex);
		queue_clear(_resultQueue);

		if (_session.IsGroupChat() && _options.groupChatMode == ChatOptions::GroupChatMode::SwapPersonas)
			SwapPersona(Role::Undefined, true);

		_contextState.EraseChat();
		SetReadyState(ReadyState::Ready);
	}

	if (seed > 0)
		Reseed(seed);

	_pStatus->EmitSignal(LLMStatusSignal::ChatInitialized);

	if (_options.flags.IsSet(ChatOptions::Flag::GreetUser))
		GreetUser();
	return true;
}

bool LLMInstance::IsInitialized() const
{
	return _readyState.load() >= ReadyState::Ready;
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

	ClearTasksQueue();
	SetReadyState(ReadyState::Ready);
	return true;
}

LLMInstance::InternalError LLMInstance::__PrepareGeneration(PrepareArguments args)
{
	// Load state
	std::scoped_lock lock(_stateMutex);

	ModelState& state = _modelState;

	auto& blocks = _contextState.GetBlocks();

	// Prepare prompt
	auto& cursor_pos = _contextState.cursor_pos;
	auto& token_pos = _contextState.token_pos;

	// Increment turn counter
	int32_t current_turn;
	if (args.progressTime)
	{
		current_turn = _turn_counter++;
		_contextState.DiscardByTTL(current_turn);
	}

	LogLn(std::format("## Turn: {}", current_turn));

	// Tokenize uncached messages
	_contextState.TokenizeUncached(_session);

	if constexpr (Disabled) // State vars
	{
		// Add state block (preface)
		if (!args.isContinuation && _options.flags.IsSet(ChatOptions::Flag::StateVariables) && !_stateVars.IsEmpty())
		{
			auto itState = find_last_if(blocks, [](const ContextBlock& block) { return block.role == Role::User && !block.is_static() && !block.is_cached(); });

			itState = std::max(itState, blocks.size() > 1 ? std::max(blocks.end() - 2, blocks.begin()) : blocks.end());

			fig::string content;
			//= string("# Story parameters\n")
			//+ "The following parameters track the state of the story and world.\n";
			content += std::format("{}", _stateVars.GetList());
			content += "\nImportant: When events demands a parameter change, end your response with a compiled list of suggested changes.";
			content += "\nEx: <change>Param = New value</change>\n";
			content = llm_tmpl::apply_chat_template({ Message { Role::System, content } }, false);
			auto state_tokens = llama::tokenize(state.pVocab, content, false);
			auto it = blocks.insert(itState, ContextBlock {
				.role = Role::System,
				.name = "",
				.content = content,
				.tokens = state_tokens,
				.flags { ContextBlockFlag::Volatile },
				.sequenceSlots { SequenceSlot::Shared }, //! @seq
				.responseId = "",
			});
		}
	}

	// Remove discarded blocks
	if (!_contextState.RemoveDiscardedBlocks())
	{
		Panic(InternalError::InvalidContextError, "Context is in an invalid state.");
		return InternalError::InvalidContextError;
	}

	// Decode
	token_pos = _contextState.DecodeUncached();
	if (not token_pos.is_valid())
	{
		Panic(InternalError::DecodeError, "Token decode error");
		return InternalError::DecodeError;
	}

	// Reserve space for response
	int32_t ctx_reserve = Constants::Context::MaxResponseLength;
	int n_ctx_used = llama::ctx_used_cells(state.pCtx);
	if (n_ctx_used + ctx_reserve >= state.ctx_size)
	{
		int32_t allocated = _contextState.ReserveTokens(ctx_reserve, false);
		if (!allocated)
		{
			Panic(InternalError::ContextFull, "Failed to reserve enough context space to hold response.");
			return InternalError::ContextFull;
		}
	}

	std::vector<Token> pre_prompt_tokens;

	// Append assistant tokens
	if (!args.isContinuation)
	{
		auto [prelude, _] = llm_tmpl::get_chat_template_prefix_suffix(args.responder, "assistant"); //! @name?
		prelude = _session.ApplyNames(prelude, args.responder);
		auto assistant_tokens = llama::tokenize(state.pVocab, prelude, false);
		pre_prompt_tokens.insert(pre_prompt_tokens.end(), assistant_tokens.begin(), assistant_tokens.end());
	}

	// Append to batch
	_contextState.GetCache().BatchWrite(pre_prompt_tokens, { SequenceSlot::Shared }, -1, token_pos.as_int()); //! @seq_id

	// Store beginning of response (after assistant prelude)
	_contextState.prepend_pos = token_pos + (int32_t)pre_prompt_tokens.size();

	// Store response position (before assistant prelude)
	if (!args.isContinuation)
		_contextState.response_pos = token_pos;
	
	// Update status
	_pStatus->ReportModelInfo(state.modelName, state.ctx_size, n_ctx_used);

	if constexpr (Debugging)
	{
		if (!llm_util::validate_kv_cache(_contextState, 0_seq, _turn_counter.load()))
		{
			DumpContext();
			Panic(InternalError::InvalidContextError, "Context desync error");
			return InternalError::InvalidContextError;
		}
	}

	assert(llama::ctx_used_cells(_contextState.GetCtxPtr()) + Constants::Context::MaxResponseLength <= _modelState.ctx_size);
	DumpContext();
	return InternalError::NoError;
}

void LLMInstance::__Generate(std::stop_token& thread_stop, GenerateArguments args, __GenerationCompleteCallback onComplete)
{
	std::unique_lock<std::timed_mutex> stateLock(_stateMutex, std::defer_lock);
	if (!stateLock.try_lock_for(100ms))
	{
		onComplete(InternalError::UnknownError, "Failed to acquire lock");
		return;
	}

	std::vector<Token> sampled_tokens;
	ContextPtr pCtx = _modelState.pCtx;
	VocabPtr pVocab = _modelState.pVocab;

	Token sampled_token;
	fig::string partial;
	
	enum class StopReason {
		Completed = 0,
		MessageCountLimit,
		ResponseLengthLimit,
		UserHalt,
		ContextFull,
		ImpersonatingUser,
	};
	StopReason stop_reason;
	fig::string response;
	MessageType msgType = args.msgType;
	Role responderRole = args.role;
	bool isContinuation = args.flags.IsSet(GenerateFlag::Continuation);
	bool isInstigation = args.flags.IsSet(GenerateFlag::Instigation);
	bool isGroupChat = _session.IsGroupChat();

	fig::string responseId = args.responseId.empty() ? CreateStrUUID() : args.responseId;
	fig::string subMessageId = args.subMessageId.empty() ? CreateStrUUID() : args.subMessageId;
	fig::string userName = _session.GetNameOf(Role::User);

	auto& response_pos = _contextState.response_pos; 
	auto& cursor_pos = _contextState.cursor_pos;
	auto& token_pos = _contextState.token_pos;
	auto& cache = _contextState.GetCache();

	int numMessages = 0;
	fig::string responderId {};
	fig::string stateReport {};
	int32_t current_turn = _turn_counter.load();

	int32_t current_sequence_index;
	SequenceSlots current_sequence;
	std::vector<int32_t> current_sequence_indices;

	if (isGroupChat and args.groupChatMode == ChatOptions::GroupChatMode::SwapSequences)
	{
		// Select sequence
		int32_t bot_index = get_bot_index(responderRole);
		if (bot_index >= 0)
			current_sequence_index = bot_index;
		else if (is_npc(responderRole))
		{
			// Keep last sequence
			current_sequence_index = _contextState.last_sequence_index >= 0 ? _contextState.last_sequence_index : 0;
		}
		else
		{
			// Use role in most recent bot response
			auto itLast = std::find_if(_contextState.GetBlocks().crbegin(), _contextState.GetBlocks().crend(), [](const ContextBlock& b) { return is_bot(b.role); });
			if (itLast != _contextState.GetBlocks().crend())
				responderRole = (*itLast).role;
			else
				responderRole = Role::Bot1;
			current_sequence_index = get_bot_index(responderRole);
		}

		current_sequence = fig::llm_util::get_sequence_from_index(current_sequence_index);
		current_sequence_indices = fig::llm_util::get_sequence_indices(current_sequence, _contextState.GetNumSequences());
	}
	else
	{
		current_sequence = SequenceSlot::Default;
		current_sequence_index = 0;
		current_sequence_indices = { 0 };
	}

	assert(current_sequence_index >= 0 && current_sequence_index < Constants::Context::MaxSequences);
	assert(llama::ctx_used_cells(pCtx) + Constants::Context::MaxResponseLength <= _modelState.ctx_size);

	if (!args.history.empty() && _modelState.pEmbedding)
	{
		LogLn(">> SEARCH EMBEDDINGS");
		_modelState.pEmbedding->Search(args.history, true, true);
	}

	LogLn(">> BEGIN GENERATION");

	// Select and init grammar
	GrammarFlags grammarFlags {};
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
			grammarFlags = grammarFlags | GrammarFlag::Narrate | GrammarFlag::EnableNarrator;
		else
			throw new std::runtime_error("AAah!");
	}
	else
		grammarFlags = GrammarFlag::Default;

	if (args.flags.IsSet(GenerateFlag::AllowNarrator))
		grammarFlags = grammarFlags | GrammarFlag::EnableNarrator;
	if (_options.flags.IsSet(ChatOptions::Flag::StateVariables))
		grammarFlags = grammarFlags | GrammarFlag::EnableState;

	CompileGrammar(grammarFlags);
	if (auto pGrammar = _modelState.SetActiveGrammar(grammarFlags))
		llama_sampler_reset(pGrammar);

	// Add prepend string
	if (!args.prepend.empty())
	{
		_contextState.Prepend(current_sequence, args.prepend);
		partial += args.prepend;
	}

	cache.InitLogits();

	auto startTime = std::chrono::steady_clock::now();

	while (true) // Main sampling loop
	{
		bool next_token = true;

		if (thread_stop.stop_requested())
		{
			stop_reason = StopReason::UserHalt;
			break; // Cancelled
		}

		if (cursor_pos >= _modelState.ctx_size)
		{
			stop_reason = StopReason::ContextFull;
			break; // Max limit reached
		}

		// Create batch view
		Batch batch_view = _contextState.GetCursorView();

		if (batch_view.n_tokens > 1) // First only
		{
			assert(token_pos == batch_view.pos[0]);
			fig::llm_util::dump_batch_tokens(batch_view, batch_view.n_tokens, current_sequence_index, pVocab, "batch.txt");
		}

		// Ensure enough space in the context to evaluate this batch
		int n_ctx_used = llama::ctx_used_cells(pCtx);
		if (n_ctx_used + batch_view.n_tokens > _modelState.ctx_size)
		{
			onComplete(InternalError::ContextFull, std::format("context size exceeded ({} / {})", n_ctx_used, _modelState.ctx_size));
			return;
		}

		if (batch_view.n_tokens > 0)
		{
			int r = llama_decode(pCtx, batch_view);
			if (r == 0) // Success
			{
				cursor_pos.increment(batch_view.n_tokens);
				token_pos.increment(batch_view.n_tokens);
			}
			else if (r < 0) // Error
			{
				onComplete(InternalError::DecodeError, "llama_decode returned error");
				return;
			}
			else if (r > 0) // Insufficient cache size (possibly due to fragmentation)
			{
				LogLn(">> REBUILDING CONTEXT");
				if (!RebuildKVCache())
				{
					onComplete(InternalError::DecodeError, "llama_decode returned error");
					return;
				}
			}
		}

		// sample the next token
		try
		{
			sampled_token = llama_sampler_sample(_modelState.pSampler, pCtx, -1);
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
		fig::string str_token;
		fig::string stop_word;
		if (!llama_vocab_is_eog(pVocab, sampled_token))
		{
			// convert the token to a string, print it and add it to the response
			str_token = llama::untokenize(pVocab, sampled_token);
			if (str_token.size() == 0)
				break; // Error

			partial += str_token;
			sampled_tokens.push_back(sampled_token);

			// check if there is incomplete UTF-8 character at the end
			bHalt = false;
			bWait = false;
			fig::llm_util::process(partial, str_token, &bWait, &bHalt, stop_word);

			if (bHalt)
				stop_reason = StopReason::Completed;
		}
		else // EOG token
		{
			bHalt = true;
			stop_reason = StopReason::Completed;
		}

		if (sampled_tokens.size() >= Constants::Context::MaxResponseLength || cursor_pos >= _modelState.ctx_size - 1)
		{
			bHalt = true;
			stop_reason = StopReason::ResponseLengthLimit;
		}

		bSend &= !bWait;
		next_token &= !bHalt;

		// Send/Queue result
		if (bSend)
		{
			fig::string carryOver;
			fig::string sendMsg = partial;
			bool bEndOfMessageType = false;

			// Check and erase formatting tags
			size_t fmt_start = partial.find('<');
			if (fmt_start != fig::npos)
			{
				bool bRemove = false;
				size_t fmt_end = partial.find('>', fmt_start + 1);
				if (fmt_end != fig::npos)
				{
					fig::string tag, tagName;
					fig::llm_util::get_tag_and_name(partial.substr(fmt_start, fmt_end - fmt_start + 1), tag, tagName);

					if (tagName == "@USR" || tagName == _session.GetNameOf(Role::User) && args.role != Role::User)
					{
						stop_reason = StopReason::ImpersonatingUser;
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
							responderId = fig::llm_util::format_id(tagName);
							responderRole = _session.GetRoleOf(responderId);
						}
						else // No remainder: New message
						{
							sendMsg.erase(fmt_start, fmt_end - fmt_start + 1);
							responderId = fig::llm_util::format_id(tagName);
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
								stop_reason = StopReason::MessageCountLimit;
								break; // That's enough, thank you
							}

							if (msgType == MessageType::Narration)
								_next_narrator_turn = current_turn + _narratorCooldownDuration;

							if (isGroupChat && is_bot(responderRole) && _contextState.active_persona != responderRole)
							{
								// Swap sequence
								if (args.groupChatMode == ChatOptions::GroupChatMode::SwapSequences)
								{
									_contextState.active_persona = responderRole;
									int32_t bot_index = get_bot_index(responderRole);
									SequenceSlots new_sequence = fig::llm_util::get_sequence_from_index(bot_index);
									if (!new_sequence.IsEmpty() && new_sequence != current_sequence)
									{
										int32_t prev_sequence_index = current_sequence_index;
										current_sequence = new_sequence;
										current_sequence_indices = fig::llm_util::get_sequence_indices(current_sequence, _contextState.GetNumSequences());
										current_sequence_index = current_sequence_indices[0];

										// Swap response over
										llama::ctx_copy_sequence(pCtx, prev_sequence_index, current_sequence_index, response_pos.as_int(), token_pos.as_int());
										cache.BatchSetSequences(response_pos.as_int(), token_pos.as_int(), current_sequence);

										LogLn(std::format(">> Sequence -> {}", current_sequence_index));
									}
								}

								// Swap persona
								else if (args.groupChatMode == ChatOptions::GroupChatMode::SwapPersonas)
								{
									if (not SwapPersona(responderRole, true))
									{
										onComplete(InternalError::PersonaSwapError, "An error occurred while swapping personas.");
										return;
									}
								}
							}
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
				LockAndDo([&]() {
					_resultQueue.push(MessagePiece {
						.responseId = responseId,
						.subMessageId = subMessageId,
						.identifier = responderId,
						.content = sendMsg,
						.role = responderRole,
						.msgType = msgType,
						.isComplete = bEndOfMessageType,
						});
				}, _resultMutex);
				response += partial;
			}

			partial = carryOver;
			if (bEndOfMessageType)
			{
				msgType = MessageType::Undefined;
				subMessageId = CreateStrUUID();
				_pStatus->EmitSignal(LLMStatusSignal::CompletedMessage);
			}
		}

		// Print to console
		Log(str_token);
		assert(cursor_pos < _modelState.ctx_size);

		// Add sampled token to batch
		cache.BatchAddSingle(sampled_token, current_sequence_indices, token_pos.as_int());

		// prepare the next batch with the sampled token
		if (!next_token)
			break; // TODO: Carry over?
	}

	// Perf
	auto endTime = std::chrono::steady_clock::now();
	if (!sampled_tokens.empty())
	{
		double duration = toD(std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count());
		_pStatus->ReportTokensPerSec(toD(sampled_tokens.size()) / (duration / 1000.0));
	}

	LogLn();
	LogLn(std::format("END OF GENERATION (reason: 0x{:02X})", (int32_t)stop_reason));
	
	LogLn(std::format("Generated {} tokens ({})", sampled_tokens.size(), token_pos));

	// Remove full response from cache (will be reinserted on next turn)
	_contextState.ClearTokensBelow(response_pos.as_int());

//	batch.n_tokens = response_pos;
	cursor_pos = _contextState.GetCache().length();
	token_pos = response_pos;
	_contextState.last_sequence_index = current_sequence_index;


	fig::llm_util::sanitize_response(response);

	if (response.length() > 0)
	{
		if (isContinuation)
		{
			auto& blocks = _contextState.GetBlocks();
			auto& lastBlock = blocks[blocks.size() - 1];
			lastBlock.content += response;
			lastBlock.tokens.insert(lastBlock.tokens.end(), sampled_tokens.begin(), sampled_tokens.end());
		}
		else
		{
			_contextState.AppendBlock(ContextBlock {
				.role = responderRole,
				.name = responderId,
				.content = response,
				.flags {}, // uncached
				.sequenceSlots { SequenceSlot::Shared },
				.turn = current_turn,
				.responseId = responseId,
			});
		}
	}
	else
	{
		Log("(Empty response)");
	}

	if constexpr (Disabled)
	{
		std::map<fig::string, fig::string> variables;
		_stateVars.UpdateValues(stateReport, variables);

		// Report state variable changes
		if (!variables.empty() && _options.flags.IsSet(ChatOptions::Flag::ReportStateChanges))
		{
			fig::string varText;
			varText.reserve(512);
			for (auto& kvp : variables)
				varText = varText + std::format("{} = {}\n", kvp.first, kvp.second);
			varText = rtrim(varText);

			_resultQueue.push(MessagePiece {
				.responseId = CreateStrUUID(),
				.subMessageId = CreateStrUUID(),
				.identifier = "",
				.content = varText,
				.role = Role::System,
				.msgType = MessageType::SystemMessage,
				.isComplete = true,
			});
		}
	}

	if constexpr (Debugging)
		llm_util::validate_kv_cache(_contextState, 0_seq, _turn_counter.load());
	onComplete(InternalError::NoError, response);
};

bool LLMInstance::__ExectuteNextTask(PrepareArguments& prepareArgs, GenerateArguments& generateArgs)
{
	LLMTask task;
	if (!LockAndReturn<bool>([&]()
	{
		if (_tasks.empty())
			return false;
		task = _tasks.front();
		_tasks.pop();
		return true;
	}, _taskMutex))
	{
		return false;
	}
	bool r;
	switch (task.type)
	{
	case LLMTaskType::SendMessage:
		r = __SendMessage(task.input, prepareArgs, generateArgs);
		break;
	case LLMTaskType::PushMessage:
		r = __PushMessage(task.role, task.input, task.msgType, !task.flags.IsSet(LLMTaskFlag::HiddenMessage), task.ttl);
		break;
	case LLMTaskType::Instigate:
		r = __Instigate(task.role, task.msgType, task.msgCount, prepareArgs, generateArgs);
		break;
	case LLMTaskType::Continue:
		r = __Continue(task.responseId, task.subMessageId, task.flags.IsSet(LLMTaskFlag::ExtendMessage), prepareArgs, generateArgs);
		break;
	default:
		return false; // Undefined
	}

	return true;
}

void LLMInstance::__ProcessTaskQueue(std::stop_token thread_stop, __GenerationCompleteCallback onComplete)
{
	InternalError error = InternalError::NoError;
	fig::string response;
	bool bWaiting = false;

	while(true)
	{
		if (thread_stop.stop_requested())
			return;

		if (error != InternalError::NoError)
		{
			onComplete(error, response);
			return;
		}

		if (bWaiting)
		{
			std::this_thread::sleep_for(50ms);
			continue;
		}

		PrepareArguments prepareArgs {};
		GenerateArguments generateArgs {};
		if (!__ExectuteNextTask(prepareArgs, generateArgs))
		{
			onComplete(InternalError::NoError, response);
			return;
		}

		if (!generateArgs.flags.IsSet(LLMInstance::GenerateFlag::Generate))
			continue;

			generateArgs.groupChatMode = _options.groupChatMode;

		// Generate response
		if (_options.flags.IsSet(ChatOptions::Flag::RandomizeMessageCount))
		{
			static std::uniform_int_distribution<int> numMessages(1, 3);
			LockAndDo([&]() {
				if (generateArgs.maxMessages <= 0) // Randomize number of messages
					generateArgs.maxMessages = numMessages(_modelState.rng); //! Hmm..
			}, _stateMutex);
		}
		else if (!_options.flags.IsSet(ChatOptions::Flag::LimitMessages))
			generateArgs.maxMessages = 0;

		if (generateArgs.responseId.empty())
			generateArgs.responseId = CreateStrUUID();
		if (generateArgs.subMessageId.empty())
			generateArgs.subMessageId = CreateStrUUID();

		_activeResponseIds.insert(generateArgs.responseId);

		bWaiting = true;
		auto prepareError = __PrepareGeneration(prepareArgs);
		if (prepareError != InternalError::NoError)
		{
			onComplete(prepareError, "");
			return;
		}

		__Generate(thread_stop, generateArgs,
			[this, &bWaiting, &error, &response](InternalError result, fig::string msg) {
				if (result != InternalError::NoError)
					error = result;
				response = msg;
				bWaiting = false;
			});
	}
}

void LLMInstance::StartGeneration()
{
	if (IsGenerating())
		return; // Already running

	SetReadyState(ReadyState::Generating);
	_pStatus->EmitSignal(LLMStatusSignal::GenerationStarted);

	_workerThread = std::make_unique<std::jthread>(std::jthread(std::bind_front(&LLMInstance::__ProcessTaskQueue, this),
		[this](InternalError error, fig::string response) {
			// ...
			if (error != InternalError::NoError)
			{
				Panic(error, response);
				return;
			}

			SetReadyState(ReadyState::Ready);
			RefreshActiveResponses();
			_pStatus->EmitSignal(LLMStatusSignal::GenerationComplete);
		}));
}

void LLMInstance::ClearResponseQueue()
{
	LockAndDo([this]() {
		queue_clear(_resultQueue);
	}, _resultMutex);
}

bool LLMInstance::EnqueueTask(LLMTask task)
{
	if (!IsReady())
		return false;

	LockAndDo([this, task]() {
		_tasks.push(task);
	}, _taskMutex);

	StartGeneration();
	return true;
}

bool LLMInstance::ClearTasksQueue()
{
	return LockAndReturn<bool>([this]() -> bool 
	{
		if (_tasks.empty())
			return false;

		queue_clear(_tasks);
		return true;
	}, _taskMutex);
}

bool LLMInstance::SendMessage(fig::string message)
{
	if (empty_or_whitespace(message))
		return false;

	return EnqueueTask(LLMTask {
		.type = LLMTaskType::SendMessage,
		.input = message,
	});
}

bool LLMInstance::PushMessage(Role role, fig::string message, MessageType msgType, bool visible, int ttl)
{
	if (empty_or_whitespace(message))
		return false;

	return EnqueueTask(LLMTask {
		.type = LLMTaskType::PushMessage,
		.input = message,
		.role = role,
		.msgType = msgType,
		.flags { visible ? LLMTaskFlag::None : LLMTaskFlag::HiddenMessage },
		.msgCount = 0,
		.ttl = ttl,
	});
}

bool LLMInstance::Instigate(Role role, MessageType msgType, int messageCount)
{
//	if (role == Role::Undefined)
//		role = Role::Bot1;

	return EnqueueTask(LLMTask {
		.type = LLMTaskType::Instigate,
		.input = "",
		.role = role,
		.msgType = msgType,
		.flags { LLMTaskFlag::HiddenMessage },
		.msgCount = messageCount,
	});
}

bool LLMInstance::Continue(fig::string responseId, fig::string subMessageId, bool extend)
{
	if (!CanGenerate())
		return false;

	return EnqueueTask(LLMTask {
		.type = LLMTaskType::Continue,
		.responseId = responseId,
		.subMessageId = subMessageId,
		.flags { LLMTaskFlag::ExtendMessage },
		.msgCount = 1,
	});
}

bool LLMInstance::__SendMessage(fig::string message, PrepareArguments& prepareArgs, GenerateArguments& generateArgs)
{
	if (empty_or_whitespace(message))
		return false;

	__PushMessage(Role::User, message, MessageType::Undefined, true, 0);

	prepareArgs = PrepareArguments {
		.responder = Role::Undefined,
		.isContinuation = false,
		.progressTime = true,
	};

	GenerateFlags flags { GenerateFlag::Generate };
	if (_next_narrator_turn < _turn_counter.load())
		flags.Set(GenerateFlag::AllowNarrator);

	generateArgs = GenerateArguments {
		.role = Role::Undefined,
		.msgType = MessageType::Undefined,
		.flags = flags,
	};
	generateArgs.history = GetHistory(Constants::Embedding::Depth);
	return true;
}

bool LLMInstance::__PushMessage(Role role, fig::string message, MessageType msgType, bool visible, int32_t ttl)
{
	if (empty_or_whitespace(message))
		return false;

	// Process
	fig::string identifier = _options.flags.IsSet(ChatOptions::Flag::UseCharacterIds) ? "@" +_session.GetIdentifierOf(role) : _session.GetNameOf(role);
	fig::string content = message;
	content = _session.ApplyNames(content);
	std::vector<Submessage> subMessages;
	content = fig::llm_util::process_message(content, identifier, &subMessages);

	if (msgType == MessageType::Undefined)
		msgType = fig::llm_util::detect_message_type(content).first;
	
	if (msgType == MessageType::SystemMessage) //! @correctness
		role = Role::System;
	else if (msgType == MessageType::Narration)
		role = Role::Narrator;
	else if (msgType == MessageType::Direction)
		role = Role::Director;

	fig::string responseId = CreateStrUUID();
	int32_t current_turn = _turn_counter.load();

	LockAndDo([&]() {
		_contextState.AppendBlock(ContextBlock {
			.role = role,
			.name = identifier,
			.content = content,
			.tokens {},
			.flags {},
			.sequenceSlots { SequenceSlot::Shared }, //! @seq
			.turn = current_turn,
			.ttl = ttl > 0 ? ttl + 1 : 0,
			.responseId = responseId,
		});
	}, _stateMutex);

	LockAndDo([&]() {
		_activeResponseIds.insert(responseId);
		if (visible)
		{
			// Add message to result queue
			for (auto const& subMsg : subMessages)
			{
				fig::string subMessageId = CreateStrUUID();
				_resultQueue.push(MessagePiece {
					.responseId = responseId,
					.subMessageId = subMessageId,
					.identifier = identifier,
					.content = subMsg.content,
					.role = role,
					.msgType = subMsg.msgType,
					.isComplete = true,
				});
			}
		}
	}, _resultMutex);
	return true;
}

bool LLMInstance::__Instigate(Role role, MessageType msgType, int messageCount, PrepareArguments& prepareArgs, GenerateArguments& generateArgs)
{	
	if (role == Role::Undefined)
		role = Role::Bot1;

	prepareArgs = PrepareArguments {
		.responder = role,
		.isContinuation = false,
		.progressTime = true,
	};

	int32_t current_turn = _turn_counter.load();
	fig::string responder = _options.flags.IsSet(ChatOptions::Flag::UseCharacterIds) ? "@" + _session.GetIdentifierOf(role) : _session.GetNameOf(role);

	bool bAllowNarration = true;

	fig::string prependMsg;
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
		_next_narrator_turn = current_turn + _narratorCooldownDuration;
	}
	else if (msgType == MessageType::Direction)
	{
		prependMsg = std::format("<{}>", Constants::Chat::DirectionTag);
		bAllowNarration = false;
	}

	bAllowNarration &= _next_narrator_turn < current_turn || msgType == MessageType::Narration;

	generateArgs = GenerateArguments {
		.role = role,
		.msgType = msgType,
		.flags { GenerateFlag::Generate, GenerateFlag::Instigation, bAllowNarration ? GenerateFlag::AllowNarrator : GenerateFlag::None },
		.maxMessages = messageCount,
		.prepend = prependMsg,
	};
	generateArgs.history = GetHistory(Constants::Embedding::Depth);
	return true;
}

bool LLMInstance::__Continue(fig::string responseId, fig::string subMessageId, bool extend, PrepareArguments& prepareArgs, GenerateArguments& generateArgs)
{
	auto& blocks = _contextState.GetBlocks();
	if (blocks.size() == 0)
		return false; // Nothing to continue

	ContextBlock& currBlock = blocks[blocks.size() - 1];
	if (currBlock.responseId != responseId)
		return false; // Not last message

	auto [msgType, bComplete] = fig::llm_util::detect_message_type(currBlock.content);
	if (msgType == MessageType::Undefined || (bComplete && !extend))
		return false; // Not incomplete message

	currBlock.Discard();

	ContextBlock newBlock = currBlock;
	newBlock.flags.Unset({ ContextBlockFlag::Cached, ContextBlockFlag::Discard });
	newBlock.flags.Set(ContextBlockFlag::Contination);

	if (extend && bComplete)
	{
		// Strip end tag
		size_t pos_end = newBlock.content.rfind("</", fig::npos);
		if (pos_end != fig::npos)
		{
			newBlock.content = newBlock.content.substr(0, pos_end);
			char last_char = newBlock.content.back();
			if (last_char == '*' || last_char == '"' || last_char == ']')
				newBlock.content.pop_back(); // Trim scaffolding char
		}
	}
	_contextState.AppendBlock(newBlock); // Reinsert block
	_contextState.response_pos = _contextState.token_pos; // Beginning of continued message

	prepareArgs = PrepareArguments {
		.responder = newBlock.role,
		.isContinuation = true,
	};

	generateArgs = GenerateArguments {
		.role = newBlock.role,
		.msgType = MessageType::Narration,
		.flags { GenerateFlag::Generate, GenerateFlag::Continuation},
		.maxMessages = 1,
		.prepend {},
		.responseId = responseId,
		.subMessageId = subMessageId,
	};

	return true;
}

bool LLMInstance::GreetUser()
{
	if (auto text = ReadTextFile("./resources/prompting/prompt_greeting.txt"))
	{
		fig::string greetingInstruction = _session.ApplyNames(text.value());
		PushMessage(Role::Director, Direction(greetingInstruction), MessageType::Direction, false, 1);
		Instigate(Role::Narrator, MessageType::Narration, 1);
		Instigate(Role::Undefined, MessageType::Dialogue, 3);
		return true;
	}
	return false;
}

bool LLMInstance::Instruct(fig::string instructions)
{
	if (auto text = ReadTextFile("./resources/prompting/prompt_formatting_director.txt"))
	{
		fig::string prompt = text.value();
		prompt = _session.ApplyNames(prompt);
		PushMessage(Role::System, prompt, MessageType::SystemMessage, false, 1);
	}

	PushMessage(Role::Director, Direction(instructions), MessageType::Direction, false, 4);
	Instigate(Role::Undefined, MessageType::Undefined, 3);
	return true;
}

int32_t LLMInstance::RewindTime(int32_t rewind_turns)
{
	int32_t turn = _turn_counter.load();
	turn = std::max(turn - rewind_turns, 0);
	_turn_counter.store(turn);
	return turn;
}

std::vector<RemovedMessage> LLMInstance::EraseMessages(int numMessages)
{
	if (!CanGenerate() || numMessages < 1)
		return {};

	std::unique_lock lock(_stateMutex, std::defer_lock);
	if (!lock.try_lock_for(50ms))
		return {};

	int32_t numRemovals = toI(std::min(toUZ(numMessages), _contextState.GetBlocks().size()));
	size_t newSize = toUZ(std::max(toI(_contextState.GetBlocks().size()) - numMessages, 0));
	auto& blocks = _contextState.GetBlocks();

	// Store removed message ids
	std::vector<RemovedMessage> removedIds;
	removedIds.reserve(_contextState.GetBlocks().size() - newSize);
	for (size_t i = newSize; i < _contextState.GetBlocks().size(); ++i)
	{
		auto const& block = blocks[i];
		blocks[i].Discard();
		removedIds.push_back(RemovedMessage {
			block.responseId,
			block.content,
			block.role,
		});
	}
	return removedIds;
}

std::vector<RemovedMessage> LLMInstance::RollbackUserMessage()
{
	if (!CanGenerate())
		return {};
	//! Disabled
	/*
	std::scoped_lock lock(_stateMutex);
	
	auto& blocks = _contextState.GetBlocks();
	for (int i = (int32_t)blocks.size() - 1; i >= 0; --i)
	{
		if (blocks[i].role == Role::User)
		{
			RewindTime(1);
			return impl_RemoveMessages((int32_t)blocks.size() - i);
		}
	} */
	return {};
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

void LLMInstance::DumpContext() const
{
#if _DEBUG
	for (int32_t i = 0; i < _contextState.GetNumSequences(); ++i)
	{
		fig::llm_util::dump_batch_text(_contextState, i, std::format("prompt_text_{}.txt", i));
		fig::llm_util::dump_batch_tokens(_contextState, i, std::format("prompt_full_{}.txt", i));
		fig::llm_util::dump_kv_cache(_contextState, i, std::format("kvcache_{}.txt", i));
	}
	fig::llm_util::dump_kv_cache_cells(_contextState, "kvcache_alloc.txt");
#endif 
}

bool LLMInstance::Reseed(uint32_t seed)
{
	if (!CanGenerate())
		return false;

	std::scoped_lock lock(_stateMutex);
	ModelState& state = _modelState;

	SamplerPtr pChain = state.pSampler;
	int n = llama_sampler_chain_n(pChain);
	SamplerPtr pDistSampler = llama_sampler_chain_get(pChain, n - 1);
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

std::set<fig::string> LLMInstance::GetActiveMessages()
{
	std::scoped_lock _ { _resultMutex };
	return std::set<fig::string>(_activeResponseIds.begin(), _activeResponseIds.end()); // Copy
}

void LLMInstance::RefreshActiveResponses()
{
	LockAndDo([&]() {
		_activeResponseIds.clear();
		for (const auto& block : _contextState.GetBlocks())
			_activeResponseIds.insert(block.responseId);
	}, _resultMutex);
}

bool LLMInstance::SwapPersona(Role role, bool immediate)
{
	int32_t turn = _turn_counter.load();

	if (!(is_bot(role) || role == Role::Undefined))
		return false;

	if (role == _contextState.active_persona)
		return true; // No change

	auto itFind = _contextState.personas.find(role);
	if (itFind == _contextState.personas.end())
		return false; // Not found;

	auto const& new_persona_tokens = itFind->second;
	int32_t new_persona_length = toI(new_persona_tokens.size());

	auto [batch_ref, batch_n] = _contextState.GetCache().GetBatch();
	auto pCtx = _contextState.GetCtxPtr();

	// Remove current persona
	auto& blocks = _contextState.GetBlocks();
	size_t persona_idx = find_index(blocks, [](const ContextBlock& b) { return b.flags.IsSet(ContextBlockFlag::Persona); });
	if (persona_idx == fig::npos)
		return false; // No persona block

	LogLn();
	LogLn(std::format(">> Swapping persona -> {}", get_bot_index(role)));

	ContextBlock& prev_persona_block = blocks[persona_idx];
	int32_t persona_cache_pos = prev_persona_block.cache_position;
	int32_t persona_attn_pos = prev_persona_block.attn_position;
	int32_t prev_persona_length = prev_persona_block.length();

	if (new_persona_length > prev_persona_length)
		DumpContext();

	// Remove existing persona
	if (prev_persona_block.is_cached())
	{
		prev_persona_block.Discard();
		auto removed_tokens = _contextState.RemoveDiscardedBlocks();
		if (!removed_tokens)
			return false;
		if (removed_tokens < prev_persona_length)
			return false;

		int32_t shift = -removed_tokens.value();
		
		// Shift response
		int32_t response_shift = shift + toI(new_persona_tokens.size());
		llama::ctx_move(pCtx, 0_seq, _contextState.response_pos.as_int(), -1, response_shift);
		llama::ctx_defrag(pCtx);
		llama::ctx_update(pCtx);
		_contextState.chat_begin_pos.increment(shift);
		_contextState.response_pos.increment(shift);
		DumpContext();
	}
	_contextState.active_persona = Role::Undefined;

	// Insert new persona
	auto itFindInactive = _contextState.personas.find(role);
	if (itFindInactive != _contextState.personas.end())
	{
		// Ensure there's enough space to fit the new persona + response
		if (new_persona_length > prev_persona_length)
		{
			_contextState.ReserveTokens(Constants::Context::MaxResponseLength + (prev_persona_length - new_persona_length));
		}

		_contextState.InsertBlock(ContextBlock {
			.role = Role::System,
			.name = "",
			.content = _session.GetPersonaOf(role),
			.tokens = new_persona_tokens,
			.flags { ContextBlockFlag::Static, ContextBlockFlag::Persona },
			.attn_position = -1,
			.sequenceSlots { SequenceSlot::Default }
		}, persona_idx);

		auto added_tokens = _contextState.RealizeUncachedBlocks();
		if (!added_tokens)
			return false; // Error
		int32_t shift = added_tokens.value();
		_contextState.chat_begin_pos.increment(shift);
		_contextState.response_pos.increment(shift);

		_contextState.active_persona = role;

		if constexpr (Debugging)
		{
			if (!llm_util::validate_kv_cache(_contextState, 0_seq, _turn_counter.load()))
				return false;
		}
		return true;
	}
	return false;
}

Sentences LLMInstance::GetHistory(size_t depth)
{
	std::scoped_lock lock(_stateMutex);
	Sentences sentences;

	int n = 0;
	const auto& blocks = _contextState.GetBlocks();
	for (auto it = blocks.crbegin(); it != blocks.crend() && n < depth; ++it, ++n)
	{
		auto& block = *it;
		fig::string msg = trim(block.content);

		size_t pos_begin = msg.find('>', 0);
		while (pos_begin != fig::npos)
		{
			size_t pos_end = msg.find('<', pos_begin);
			if (pos_end == fig::npos)
				break;

			fig::string content = msg.substr(pos_begin + 1, pos_end - pos_begin - 1);
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
				replace_all_inplace(content, "?", ".");
				replace_all_inplace(content, "!", ".");
				auto split_sentences = split(content, '.', true);
				for (auto& s : split_sentences)
					sentences.insert(sentences.begin(), { block.role, s });
			}
			else
			{
				sentences.insert(sentences.begin(), { block.role, content });
			}

			pos_begin = msg.find('>', pos_end + 1);
			if (pos_begin == fig::npos)
				break;
			pos_begin = msg.find('>', pos_begin + 1);
		}
	}

	return sentences;
}

#if _DEBUG
bool LLMInstance::GenerateEmbedding(fig::string text)
{
	if (!_modelState.pEmbedding)
		return false;

	EmbeddingVector embedding;
	if (_modelState.pEmbedding->Generate(text, embedding))
	{
		Embeddings::AddEmbedding(embedding);

		// Save to disk
		fig::string filename = std::format("./embeddings/{}.txt", CreateStrUUID());
		embedding.SaveToFile(filename);
	}
	return true; // Break here
}
#endif

SamplerPtr LLMInstance::CompileGrammar(GrammarFlags flags)
{
	if (flags.IsEmpty())
		return nullptr;

	auto itFind = _modelState.grammars.find(flags);
	if (itFind != _modelState.grammars.end())
	{
		LogLn(std::format("Selecting grammar variant 0x{:02X}", (uint32_t)flags));
		return itFind->second;
	}

	LogLn(std::format("Compiling grammar variant 0x{:02X}", (uint32_t)flags));
	SamplerPtr pGrammar = Grammar::compile_grammar(
		flags,
		_modelState.pVocab, 
		_session.GetNameGrammar(_options.flags.IsSet(ChatOptions::Flag::UseCharacterIds), _options.flags.IsSet(ChatOptions::Flag::AllowUserResponse)),
		_stateVars.GetGrammarPattern());
	
	_modelState.grammars[flags] = pGrammar;
	return pGrammar;
}

bool LLMInstance::SetStateVariable(fig::string name, fig::string value, bool allowCreate)
{
	if (!_options.flags.IsSet(ChatOptions::Flag::StateVariables))
		return false;

	std::unique_lock<std::timed_mutex> stateLock(_stateMutex, std::defer_lock);
	if (!stateLock.try_lock_for(100ms))
		return false;

	if (!_stateVars.HasValue(name) && !allowCreate)
		return false;

	_stateVars.SetValue(name, value);
	return true;
}

std::map<fig::string, fig::string> LLMInstance::GetStateVariables()
{
	std::map<fig::string, fig::string> result;

	std::unique_lock<std::timed_mutex> stateLock(_stateMutex, std::defer_lock);
	if (stateLock.try_lock_for(100ms))
		result.insert(_stateVars.GetVariables().begin(), _stateVars.GetVariables().end());

	return result;
}

bool LLMInstance::RebuildKVCache()
{
	_pStatus->EmitSignal(LLMStatusSignal::RebuildingKVCache);
    auto prevReadyState = _readyState.exchange(ReadyState::RebuildingKVCache);
	_pStatus->ReportReadyState(ReadyState::RebuildingKVCache);

	bool r = _contextState.RebuildKVCache();

    SetReadyState(prevReadyState);
	_pStatus->EmitSignal(LLMStatusSignal::GenerationStarted);
    return r == 0;
}

void LLMInstance::SetReadyState(ReadyState readyState)
{
	_readyState.store(readyState);
	_pStatus->ReportReadyState(readyState);
}

void LLMInstance::Panic(InternalError error, const string& message)
{
	LogLn(std::format("\r\n>> Internal error 0x{:02X}: {}", toI(error), message));
	SetReadyState(ReadyState::Invalid);
	_pStatus->EmitSignal(LLMStatusSignal::ModelUnloadRequest);
}