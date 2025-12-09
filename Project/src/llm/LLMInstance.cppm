module;

#include <llama.h>
#include <cassert>
#include <cstdio>

export module LLMInstance;

import Common;

export import LLMTypes;

import ChatSession;
import LLMTemplate;
import LLMUtility;
import LLMEmbedding;
import LLMStateVariables;
import Embedding;
import ModelState;
import Grammar;

import Context;

export {

	using LoadModelCallback = std::function<void(bool)>;
	using LoadModelProgressCallback = std::function<void(int)>;

	enum class LLMStatusSignal {
		Nothing = 0,
		LoadingModel,
		LoadedModel,
		LoadModelFailure,
		UnloadedModel,
		GenerationStarted,
		GenerationComplete,
		InitializingChat,
		InitializedChat,
		InitializeChatFailure,
		CompletedMessage,
		RebuildingContext,
	};

	struct LLMStatus
	{
		string modelName;
		size_t allocCtxSize = 0;
		size_t usedCtxSize = 0;
		int64_t usedVRAM = 0;
		int64_t usedRAM = 0;
		bool bReady = false;
		bool bInvalid = false;
		double tokensPerSec = 0.0;
		LLMStatusSignal signal;
	};

	struct RemovedMessage
	{
		string responseId;
		string content;
		Role role;
	};

	struct LLMChatArguments
	{
		ChatSession session;
		Messages messages;
		LLMOptions options;
		int32_t narrationCooldownDuration = Constants::Chat::DefaultNarratorCooldown;
	};

	enum class LLMTaskFlag : int32_t
	{
		None = 0,
		HiddenMessage = 1 << 0,
	};
	using LLMTaskFlags = EnumFlags<LLMTaskFlag>;

	extern "C++" class LLMInstance
	{
	public:
		LLMInstance();
		~LLMInstance();

		bool Initialize(string filename, LLMOptions options, LoadModelProgressCallback onProgress, LoadModelCallback onComplete);
		void Shutdown();

		bool IsInitialized() const { return _modelState.pModel != nullptr; }
		bool IsInitializing() const { return _readyState.load() == ReadyState::LoadingModel; }

		bool InitializeChat(LLMChatArguments args);

		bool IsReady() const;
		bool IsGenerating() const;

		bool Halt();
		bool Continue(string responseId, string subMessageId, bool extend);

		// Tasks
		bool GreetUser();
		bool SendMessage(string message);
		bool PushMessage(Role role, string message, MessageType msgType = MessageType::Undefined, bool visible = true, int ttl = 0);
		bool Instigate(Role role, MessageType msgType, int messageCount = 0);
		bool Instruct(string instructions);

		bool ResetChat(int seed = -1);
		bool Reseed(uint32_t seed = 0xFFFFFFFF);
		std::vector<RemovedMessage> RemoveMessages(int numMessages = 1, bool rewindTime = true);
		std::vector<RemovedMessage> RollbackUserMessage();
		std::set<string> GetActiveMessages();

		bool SetStateVariable(string name, string value, bool allowCreate = true);
		bool PollResponse(MessagePiece& piece);
		std::pair<LLMStatus, bool> PollStatus();

		void DumpSequence(int32_t seq_id) const;
		void DumpContext() const;
#if _DEBUG
		bool GenerateEmbedding(string text);
#endif

		const ChatSession& GetSession() const { return _session; }
		std::map<string, string> GetStateVariables();

	private:
		void ClearResponseQueue();
		bool CanGenerate() const;

	public:
		enum class GenerateFlag : int32_t
		{
			None = 0,
			Generate = 1 << 0,
			Continuation = 1 << 1,
			Instigation = 1 << 2,
			AllowNarrator = 1 << 3,
			SwapPersonas = 1 << 4,
		};
		using GenerateFlags = EnumFlags<GenerateFlag>;

	private:
		struct __PartialResult
		{
			string piece;
			string fullText;
		};

		enum class InternalError : int {
			NoError = 0,
			ContextFull,
			DecodeError,
			SamplerError,
			GrammarError,
			UnknownError,
		};

		using __LoadModelCallback = std::function<void(ModelState)>;
		void __LoadModel(string filename, __LoadModelCallback onComplete);

		using __PartialResultCallback = std::function<void(__PartialResult)>;
		using __GenerationCompleteCallback = std::function<void(InternalError error, string msg)>;

		struct PrepareArguments
		{
			Role responder = Role::Bot1;
			bool isContinuation = false;
			int time = 0;	// decrement ttl
		};
		void __PrepareGeneration(PrepareArguments args);

		struct GenerateArguments
		{
			Role role = Role::Undefined;
			MessageType msgType = MessageType::Undefined;
			GenerateFlags flags = GenerateFlags::None;
			int maxMessages = 0;
			string prepend {};
			string responseId {};
			string subMessageId {};
			Sentences history; // Used for embedding
		};
		void __Generate(std::stop_token& stop, GenerateArguments args, __GenerationCompleteCallback onComplete);
		void StartGeneration();
		bool SwapPersona(Role persona);

		void PushSignal(LLMStatusSignal signal);
		void RefreshActiveResponses();
		std::vector<RemovedMessage> impl_RemoveMessages(int numMessages, bool rewindTime);
		bool RebuildKVCache();

		Sentences GetHistory(size_t depth);
		SamplerPtr CompileGrammar(GrammarFlags grammarFlags);
		void InitSamplers();

		// Tasks
		enum class LLMTaskType
		{
			SendMessage,
			PushMessage,
			Instigate,
			Continue,
		};

		struct LLMTask
		{
			LLMTaskType type;

			// Parameters
			string input;
			Role role = Role::Undefined;
			MessageType msgType = MessageType::Undefined;

			LLMTaskFlags flags = LLMTaskFlags::None;
			int msgCount = 0;
			int ttl = 0;
		};
		bool EnqueueTask(LLMTask task);
		bool ClearTasksQueue();

		void __ProcessTaskQueue(std::stop_token stop, __GenerationCompleteCallback onComplete);
		bool __ExectuteNextTask(PrepareArguments& prepareArgs, GenerateArguments& generateArgs);
		bool __SendMessage(string message, PrepareArguments& prepareArgs, GenerateArguments& generateArgs);
		bool __PushMessage(Role role, string message, MessageType msgType, bool visible, int ttl);
		bool __Instigate(Role role, MessageType msgType, int messageCount, PrepareArguments& prepareArgs, GenerateArguments& generateArgs);
	private:
		enum class ReadyState { Invalid, Uninitialized, LoadingModel, ModelLoaded, Initializing, Ready, Generating, RebuildingContext };
		std::atomic<ReadyState> _readyState { ReadyState::Uninitialized };

		std::timed_mutex _stateMutex; // Guards state variables
		ModelState _modelState;
		Context _contextState;

		std::mutex _resultMutex; // Guards output queue
		std::queue<MessagePiece> _resultQueue;
		std::set<string> _activeResponseIds;

		std::mutex _statusMutex; // Guards status reporting
		LLMStatus _lastStatus {};
		std::queue<LLMStatusSignal> _statusSignals;
		std::atomic<double> _tokensPerSec {};

		std::unique_ptr<std::jthread> _workerThread;

		// Tasks
		std::mutex _taskMutex; // Guards task queue
		std::queue<LLMTask> _tasks;

		// Session
		ChatSession _session;
		LLMOptions _options;
		bool _bCtxReallocateNextTurn = false;

		// Embedding
		std::unique_ptr<LLMEmbedding> _pEmbedding;

		// State
		LLMStateVariables _stateVars;
		int32_t _narratorCooldownDuration = 0;
		int32_t _narratorCooldown = 0;

	public:
		std::atomic<int64_t> usedVRAM; // As reported from llama.cpp
		std::atomic<int64_t> usedRAM; // As reported from llama.cpp
	};
} // export


template <typename T>
concept Lockable = requires (T mut)
{
	mut.try_lock();
	mut.lock();
	mut.unlock();
};

template <Lockable... MutexTypes>
static void LockAndDo(std::function<void()> fn, MutexTypes&... mutexes)
{
	std::scoped_lock _ { mutexes... };
	fn();
}

template <typename ReturnType, Lockable MutexType>
[[nodiscard]] static ReturnType LockAndReturn(std::function<ReturnType()> fn, MutexType& mutex)
{
	std::scoped_lock _ { mutex };
	return fn();
}

inline constexpr string Direction(std::string_view text)
{
	return "{{" + std::string(text) + "}}";
}

inline constexpr string Narration(std::string_view text)
{
	return "[" + std::string(text) + "]";
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

	LockAndDo([this]() {
		// Clear and release state
		_modelState.Release();
		_modelState = {};
		_contextState = {};

		if (_pEmbedding)
		{
			_pEmbedding->Shutdown();
			_pEmbedding = nullptr;
		}
	}, _stateMutex);
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
		double mul = 1024.0 * 1024.0; // MiB
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

	bool bMultiSequence = args.session.IsGroupChat() && args.options.IsSet(LLMOption::UseMultipleSequences);
	int32_t n_bots = bMultiSequence ? (int32_t)args.session.GetBotCount() : 1;
	if (n_bots == 0)
		return false; // Error

	_contextState = Context(_modelState); //!
	_session = args.session;
	_stateVars = {};

	// Read personas
	std::map<Role, string> personas;
	int32_t botCount = (int32_t)_session.GetBotCount();
	for (int32_t i = 0; i < botCount; ++i)
	{
		Role role = bot_from_index(i);
		personas[role] = _session.GetPersonaOf(role);
	}
	string user_persona = _session.GetPersonaOf(Role::User);

	if (!personas.contains(Role::Bot1))
		return false; // No main character

	_narratorCooldownDuration = args.narrationCooldownDuration;
	_narratorCooldown = -1;

	// Initialize sampler + grammar
	InitSamplers();

	// Initialize rng
#if _DEBUG
	_modelState.rng.seed(DEBUG_SEED);
#else
	_modelState.rng.seed((uint32_t)std::chrono::steady_clock::now().time_since_epoch().count());
#endif

	const llama_vocab* pVocab = llama_model_get_vocab(_modelState.pModel);

	// Init state variables (temp)
	if (args.options.IsSet(LLMOption::StateVariables))
	{
		_stateVars.SetValue("Location", "Kitchen"); //! @temp
		_stateVars.SetValue(_session.ApplyNames("{{char}}'s mood", Role::Bot1), "Neutral"); //! @temp
	}

	// Initialize context
	_contextState.Initialize();

	auto fnDecode = [this](const std::vector<llama_token>& tokens, SequenceId seq_id, int32_t& pos) -> bool {
		if (auto n_tokens = _contextState.DecodeTokens(tokens, pos, seq_id))
		{
			pos += n_tokens.value();
			return true;
		}
		return false;
	};

	auto [template_prefix, template_suffix] = llm_tmpl::get_chat_template_prefix_suffix(Role::System, "");

	if (bMultiSequence) // Initialize a sequence for each bot
	{
		// Write (shared) system_prompt
		int32_t& cursor_pos = _contextState.cursor_pos = 0;
		string system_prompt = _session.GetSystemPrompt(); //! @group

		std::vector<llama_token> system_prompt_tokens = llm_util::tokenize(pVocab, template_prefix + system_prompt, false); // <BOS>?;

		// System prompt
		_contextState.AppendBlock(ContextBlock {
			.role = Role::System,
			.name = "",
			.content = template_prefix + system_prompt,
			.tokens = system_prompt_tokens,
			.flags { ContextBlockFlag::Static },
			.sequenceId { Sequence::Shared },
			.offset = _contextState.GetBlockAppendOffset(),
			});

		if (!fnDecode(system_prompt_tokens, { Sequence::Shared }, cursor_pos))
			return false; // Error

		// Write persona(s)
		int32_t max_persona = 0;
		for (int32_t i = 0; i < n_bots; ++i)
		{
			int32_t persona_offset = cursor_pos;
			Role role = bot_from_index(i);
			SequenceId seq_id = llm_util::sequence_from_index(i);

			// Persona
			std::vector<llama_token> persona_tokens = llm_util::tokenize(pVocab, personas[role], false);
			_contextState.AppendBlock(ContextBlock {
				.role = Role::System,
				.name = "",
				.content = personas[role],
				.tokens = persona_tokens,
				.flags { ContextBlockFlag::Static,  ContextBlockFlag::Persona },
				.sequenceId = seq_id,
				.offset = persona_offset,
				});

			if (!persona_tokens.empty() && !fnDecode(persona_tokens, seq_id, persona_offset))
				return false;
			max_persona = std::max(max_persona, toI(persona_tokens.size()));
		}
		cursor_pos += max_persona;

		// User persona
		if (!string_util::empty_or_whitespace(user_persona))
		{
			auto user_persona_tokens = llm_util::tokenize(pVocab, user_persona);
			_contextState.AppendBlock(ContextBlock {
				.role = Role::System,
				.name = "",
				.content = user_persona,
				.tokens = user_persona_tokens,
				.flags { ContextBlockFlag::Static },
				.sequenceId { Sequence::Shared },
				.offset = cursor_pos,
				});
			if (!user_persona_tokens.empty() && !fnDecode(user_persona_tokens, { Sequence::Shared }, cursor_pos))
				return false;
		}

		// Template suffix
		auto template_suffix_tokens = llm_util::tokenize(pVocab, template_suffix);
		_contextState.AppendBlock(ContextBlock {
			.role = Role::System,
			.name = "",
			.content = template_suffix,
			.tokens = template_suffix_tokens,
			.flags { ContextBlockFlag::Static },
			.sequenceId { Sequence::Shared },
			.offset = cursor_pos,
			});
		if (!fnDecode(template_suffix_tokens, { Sequence::Shared }, cursor_pos))
			return false;
	}
	else // Single sequence
	{
		int32_t& cursor_pos = _contextState.cursor_pos = 0;

		if (_session.IsGroupChat())
		{
			// Tokenize and store personas for later
			for (const auto& kvp : personas)
			{
				if (string_util::empty_or_whitespace(kvp.second))
					continue;

				_contextState.personas[kvp.first] = llm_util::tokenize(pVocab, kvp.second, false);
			}
		}

		string system_prompt = _session.GetSystemPrompt();
		std::vector<llama_token> system_prompt_tokens = llm_util::tokenize(pVocab, template_prefix + system_prompt, false); // <BOS>?;

		_contextState.AppendBlock(ContextBlock {
			.role = Role::System,
			.name = "",
			.content = template_prefix + system_prompt,
			.tokens = system_prompt_tokens,
			.flags { ContextBlockFlag::Static },
			.sequenceId { Sequence::Default },
			.offset = 0,
			});

		if (!fnDecode(system_prompt_tokens, { Sequence::Default }, cursor_pos))
			return false;

		if (!_session.IsGroupChat())
		{
			// Tokenize persona(s)
			std::vector<llama_token> persona_tokens = llm_util::tokenize(pVocab, personas[Role::Bot1], false);
			_contextState.AppendBlock(ContextBlock {
				.role = Role::System,
				.name = "",
				.content = personas[Role::Bot1],
				.tokens = persona_tokens,
				.flags { ContextBlockFlag::Static },
				.sequenceId { Sequence::Default },
				.offset = cursor_pos,
				});
			if (!persona_tokens.empty() && !fnDecode(persona_tokens, { Sequence::Default }, cursor_pos))
				return false;
		}

		// User persona
		if (!string_util::empty_or_whitespace(user_persona))
		{
			auto user_persona_tokens = llm_util::tokenize(pVocab, user_persona);
			_contextState.AppendBlock(ContextBlock {
				.role = Role::System,
				.name = "",
				.content = user_persona,
				.tokens = user_persona_tokens,
				.flags { ContextBlockFlag::Static },
				.sequenceId { Sequence::Default },
				.offset = cursor_pos,
				});
			if (!user_persona_tokens.empty() && !fnDecode(user_persona_tokens, { Sequence::Shared }, cursor_pos))
				return false;
		}

		// Template suffix
		auto template_suffix_tokens = llm_util::tokenize(pVocab, template_suffix);
		_contextState.AppendBlock(ContextBlock {
			.role = Role::System,
			.name = "",
			.content = template_suffix,
			.tokens = template_suffix_tokens,
			.flags { ContextBlockFlag::Static },
			.sequenceId { Sequence::Default },
			.offset = cursor_pos,
			});
		if (!fnDecode(template_suffix_tokens, { Sequence::Shared }, cursor_pos)) //! Shared?
			return false;
	}

	for (auto& block : _contextState.GetBlocks())
	{
		block.flags = block.flags | ContextBlockFlag::Cached;
		_contextState.chat_begin_pos = _contextState.GetBlockAppendOffset();
	}

	_bCtxReallocateNextTurn = false;
	_readyState.store(ReadyState::Ready);
	PushSignal(LLMStatusSignal::InitializedChat);
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
	string grammar = ReadTextFile("./resources/grammar/formatting_grammar.gbnf").value_or("");

	// Names
	string namesPattern;
	int32_t botCount = (int32_t)_session.GetBotCount();
	for (int i = 0; i < botCount; ++i)
	{
		if (i > 0)
			namesPattern += "| ";
		if (_options.IsSet(LLMOption::UseCharacterIds))
			namesPattern += std::format("| \"@{}\"", _session.GetIdentifierOf(bot_from_index(i)));
		else
			namesPattern += std::format("| \"{}\"", _session.GetNameOf(bot_from_index(i)));
	}
	if (_options.IsSet(LLMOption::AllowUserResponse))
	{
		if (_options.IsSet(LLMOption::UseCharacterIds))
			namesPattern += std::format("| \"@{}\"", _session.GetIdentifierOf(Role::User));
		else
			namesPattern += std::format("| \"{}\"", _session.GetNameOf(Role::User));
	}
	string_util::replace_all(grammar, "##NAMES##", namesPattern);

	// Variables
	if (_options.IsSet(LLMOption::StateVariables))
	{
		string_util::replace_all(grammar, "##STATE##", "stat");
		string_util::replace_all(grammar, "##STATE_VARS##", _stateVars.GetGrammarPattern());
	}
	else
	{
		string_util::replace_all(grammar, "##STATE##", "");
		string_util::replace_all(grammar, "##STATE_VARS##", "[]");
	}

	auto default_grammar_sampler = CompileGrammar({ GrammarFlag::Default });

	if (default_grammar_sampler) llama_sampler_chain_add(pSampler, default_grammar_sampler);	// Grammar
	llama_sampler_chain_add(pSampler, llama_sampler_init_min_p(0.15f, 1));						// Min P sampler
	llama_sampler_chain_add(pSampler, llama_sampler_init_temp(2.5f));							// Temperature
	llama_sampler_chain_add(pSampler, llama_sampler_init_penalties(512, 1.05f, 0.0f, 0.0f));	// Repeat penalty
#if _DEBUG
	llama_sampler_chain_add(pSampler, llama_sampler_init_dist(DEBUG_SEED));						// Seed
#else
	llama_sampler_chain_add(pSampler, llama_sampler_init_dist(DEFAULT_SEED));				// Seed
#endif

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
		ClearQueue(_resultQueue);

		if (_session.IsGroupChat() && !_options.IsSet(LLMOption::UseMultipleSequences))
			SwapPersona(Role::Undefined);

		_contextState.EraseChat();
		_readyState.store(ReadyState::Ready);
	}

	if (seed > 0)
		Reseed(seed);

	PushSignal(LLMStatusSignal::InitializedChat);

	if (_options.IsSet(LLMOption::GreetUser))
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

	// initialize the context
	llama_context_params ctx_params = llama_context_default_params();
	int32_t n_ctx = std::min(llama_model_n_ctx_train(state.pModel), Constants::Context::DefaultSize);
	int32_t n_seq_max = Constants::Context::MaxSequences;
	ctx_params.n_ctx = n_ctx;
	ctx_params.n_seq_max = n_seq_max;
	ctx_params.n_batch = n_ctx;
	ctx_params.n_ubatch = Constants::Context::MicroBatchSize;

	state.pCtx = llama_init_from_model(state.pModel, ctx_params);
	state.ctx_size = llama_n_ctx(state.pCtx);
	state.num_sequences = n_seq_max;

	if (!state.pCtx)
	{
		fprintf(stderr, "%s: error: failed to create the llama_context\n", __func__);
		onComplete(state);
		return;
	}

	// Initialize embedder
	if (_options.IsSet(LLMOption::Embeddings))
	{
		_pEmbedding = std::make_unique<LLMEmbedding>();
		if (_pEmbedding->LoadModel(string(Constants::Embedding::DefaultModelLocation)))
			DebugPrintLn("Loaded embedding model");
		else
			_pEmbedding = nullptr; // Destroy
	}

	onComplete(state);
}

bool LLMInstance::Initialize(string filename, LLMOptions options, LoadModelProgressCallback onProgress, LoadModelCallback onComplete)
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
			LockAndDo([this, &result]() {
				_modelState = result;
			}, _stateMutex);
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

		auto& blocks = _contextState.GetBlocks();
		if (blocks.size() == 0)
			return false;

		ContextBlock& currBlock = blocks[blocks.size() - 1];
		if (currBlock.responseId != responseId)
			return false; // Not last message

		auto [msgType, bComplete] = llm_util::detect_message_type(currBlock.content);
		if (msgType == MessageType::Undefined || (bComplete && !extend))
			return false; // Not incomplete message

		ContextBlock block = currBlock;
		block.flags.Unset(ContextBlockFlag::Cached);
		impl_RemoveMessages(1, false); // Remove the last message, resets cursor_pos

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
		_contextState.AppendBlock(block); // Reinsert block
		_contextState.response_pos = _contextState.cursor_pos; // Beginning of continued message

		generateArgs = GenerateArguments {
			.role = block.role,
			.msgType = msgType,
			.flags { GenerateFlag::Generate, GenerateFlag::Continuation},
			.maxMessages = 1,
			.prepend {},
			.responseId = responseId,
			.subMessageId = subMessageId,
		};
	} // Release state lock

	PrepareArguments prepareArgs {
		.responder = Role::Undefined,
		.isContinuation = true,
		.time = 0,
	};
	__PrepareGeneration(prepareArgs);
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

void LLMInstance::__PrepareGeneration(PrepareArguments args)
{
	// Load state
	std::scoped_lock lock(_stateMutex);

	ModelState& state = _modelState;

	auto& blocks = _contextState.GetBlocks();

	// Prepare prompt
	int32_t& cursor_pos = _contextState.cursor_pos;
	std::vector<llama_token> last_response_tokens;

	// Remove volatile blocks
	_contextState.EraseVolatile();

	// Allocate and shift context window
	if (_bCtxReallocateNextTurn)
	{
		_contextState.ReserveTokens(Constants::Context::MicroBatchSize, true);
		_bCtxReallocateNextTurn = false;
	}

	// Decrement ttl
	_contextState.DecrementTTL(args.time);

	// Decrement narrator cooldown
	_narratorCooldown = std::max(_narratorCooldown - args.time, 0); //! Move to session?

	// Tokenize uncached messages
	int32_t offset = 0;
	for (auto& block : blocks)
	{
		if (block.is_static() || block.is_cached())
		{
			offset = std::max(offset, block.offset + block.length());
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

		block.offset = offset;
		block.tokens = llm_util::tokenize(state.pVocab, content, false);

		offset += block.length();
		last_response_tokens.insert(last_response_tokens.end(), block.tokens.cbegin(), block.tokens.cend());
	}

#if FALSE // State vars
	// Add state block (preface)
	if (!args.isContinuation && _options.IsSet(LLMOption::StateVariables) && !_stateVars.IsEmpty())
	{
		auto itUserRev = std::find_if(blocks.rbegin(), blocks.rend(), [](const ContextBlock& block) { return block.role == Role::User && !block.is_static() && !block.is_cached(); });
		auto itState = flip_iterator<ContextBlock>(blocks, itUserRev);

		itState = std::max(itState, blocks.size() > 1 ? std::max(blocks.end() - 2, blocks.begin()) : blocks.end());

		string content;
		//= string("# Story parameters\n")
		//+ "The following parameters track the state of the story and world.\n";
		content += std::format("{}", _stateVars.GetList());
		content += "\nImportant: When events demands a parameter change, end your response with a compiled list of suggested changes.";
		content += "\nEx: <change>Param = New value</change>\n";
		content = llm_tmpl::apply_chat_template({ Message { Role::System, content } }, false);
		auto state_tokens = llm_util::tokenize(state.pVocab, content, false);
		auto it = blocks.insert(itState, ContextBlock {
			.role = Role::System,
			.name = "",
			.content = content,
			.tokens = state_tokens,
			.flags = ContextBlockFlag::Volatile,
			.sequenceId = Sequence::Shared,
			.responseId = "",
			});

		_contextState.RefreshBlockPositions();

		last_response_tokens.insert(last_response_tokens.begin() + ptrdiff_t((it->offset) - cursor_pos), state_tokens.cbegin(), state_tokens.cend());
	}
#endif

	// Allocate and shift context window
	int32_t ctx_reserve = std::max((int32_t)last_response_tokens.size() + Constants::Context::MaxResponseLength, Constants::Context::MicroBatchSize);
	int n_ctx_used = llama_kv_self_used_cells(state.pCtx);
	if (n_ctx_used + ctx_reserve >= state.ctx_size)
	{
		if (_contextState.ReserveTokens(ctx_reserve, false))
			_bCtxReallocateNextTurn = false;
	}

	// Write new blocks to batch
	for (auto& block : _contextState.GetBlocks())
	{
		if (block.is_static() || block.is_cached())
			continue;

		_contextState.GetCache().BatchWrite(block.tokens, block.sequenceId, block.offset);
	}

	// Decode
	cursor_pos = _contextState.DecodeUncached(cursor_pos);

	std::vector<llama_token> pre_prompt_tokens;

	// Append assistant tokens
	if (!args.isContinuation)
	{
		auto [prelude, _] = llm_tmpl::get_chat_template_prefix_suffix(args.responder, "assistant"); //! @name?
		prelude = _session.ApplyNames(prelude, args.responder);
		auto assistant_tokens = llm_util::tokenize(state.pVocab, prelude, false);
		pre_prompt_tokens.insert(pre_prompt_tokens.end(), assistant_tokens.begin(), assistant_tokens.end());
	}

	// Append to batch
	_contextState.GetCache().BatchWrite(pre_prompt_tokens, { Sequence::Shared }, cursor_pos); //! @seq_id

	// Store beginning of response (after assistant prelude)
	_contextState.prepend_pos = cursor_pos + (int32_t)pre_prompt_tokens.size();
	// Store response position (before assistant prelude)
	if (!args.isContinuation)
		_contextState.response_pos = cursor_pos;

	//	DumpContext();
}

void LLMInstance::__Generate(std::stop_token& thread_stop, GenerateArguments args, __GenerationCompleteCallback onComplete)
{
	std::unique_lock<std::timed_mutex> stateLock(_stateMutex, std::defer_lock);
	if (!stateLock.try_lock_for(std::chrono::milliseconds(100)))
	{
		onComplete(InternalError::UnknownError, "Failed to acquire lock");
		return;
	}

	std::vector<llama_token> sampled_tokens;
	ModelState& state = _modelState;
	VocabPtr pVocab = state.pVocab;

	llama_token sampled_token;
	string partial;
	string stop_reason;
	string response;
	MessageType msgType = args.msgType;
	bool isContinuation = args.flags.IsSet(GenerateFlag::Continuation);
	bool isInstigation = args.flags.IsSet(GenerateFlag::Instigation);

	auto& blocks = _contextState.GetBlocks();
	auto& cache = _contextState.GetCache();

	auto [batch_ref, batch_n] = cache.GetBatch();
	llama_batch& batch = batch_ref.get();
	//	int32_t n_batch = llama_n_batch(state.pCtx);
	int32_t ctx_size = state.ctx_size;
	string userName = _session.GetNameOf(Role::User);

	int32_t pre_response_pos = _contextState.response_pos;
	int32_t& cursor_pos = _contextState.cursor_pos;

	string responseId = args.responseId.empty() ? CreateUUID() : args.responseId;
	string subMessageId = args.subMessageId.empty() ? CreateUUID() : args.subMessageId;

	int numMessages = 0;
	Role responderRole = args.role;
	string responderId {};
	string stateReport {};

	// Select sequence
	int32_t current_sequence_index;
	int32_t bot_index = get_bot_index(responderRole);
	if (bot_index >= 0)
		current_sequence_index = bot_index;
	else if (is_npc(responderRole))
	{
		// Keep last sequence
		current_sequence_index = _contextState.previous_sequence_index >= 0 ? _contextState.previous_sequence_index : 0;
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

	SequenceId current_sequence = llm_util::sequence_from_index(current_sequence_index);
	const int32_t n_seq_max = state.num_sequences;
	std::vector<int32_t> current_sequence_indices = llm_util::get_sequence_indices(current_sequence, state.num_sequences);

	assert(current_sequence_index >= 0 && current_sequence_index < Constants::Context::MaxSequences);
	assert(llama_kv_self_used_cells(state.pCtx) + Constants::Context::MaxResponseLength <= ctx_size);

	if (!args.history.empty() && _pEmbedding)
	{
		DebugPrintLn(">> SEARCH EMBEDDINGS");
		_pEmbedding->Search(args.history, true, true);
	}

	DebugPrintLn(">> BEGIN GENERATION");

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
			grammarFlags = grammarFlags | GrammarFlag::Narrate;
		else
			throw new std::runtime_error("AAah!");
	}
	else
		grammarFlags = GrammarFlag::Default;

	if (args.flags.IsSet(GenerateFlag::AllowNarrator))
		grammarFlags = grammarFlags | GrammarFlag::EnableNarrator;
	if (_options.IsSet(LLMOption::StateVariables))
		grammarFlags = grammarFlags | GrammarFlag::EnableState;

	CompileGrammar(grammarFlags);
	if (auto pGrammar = state.SetActiveGrammar(grammarFlags))
		llama_sampler_reset(pGrammar);

	// Add prepend string
	if (!args.prepend.empty())
	{
		auto prepend_tokens = llm_util::tokenize(state.pVocab, args.prepend, false);

		// Append to batch
		cache.BatchWrite(prepend_tokens, current_sequence, _contextState.prepend_pos);

		partial += args.prepend;
		printf("%s", partial.c_str());
	}

	cache.InitLogits();

	auto startTime = std::chrono::steady_clock::now();

	while (true)
	{
		bool next_token = true;

		if (thread_stop.stop_requested())
		{
			stop_reason = "cancel";
			break; // Cancelled
		}

		if (cursor_pos >= ctx_size)
		{
			stop_reason = "max ctx";
			break; // Max limit reached
		}

		// Create batch view
		llama_batch batch_view = cache.GetBatchView(cursor_pos, cache.length() - cursor_pos);

		if (batch_view.n_tokens > 1) // First only
			llm_util::dump_batch_tokens(batch_view, batch_view.n_tokens, current_sequence_index, state.pVocab, "batch.txt");

		// Ensure enough space in the context to evaluate this batch
		int n_ctx_used = llama_kv_self_used_cells(state.pCtx);
		if (n_ctx_used + batch_view.n_tokens > ctx_size)
		{
			onComplete(InternalError::ContextFull, std::format("context size exceeded ({} / {})", n_ctx_used, ctx_size));
			return;
		}

		if (batch_view.n_tokens > 0)
		{
			int r = llama_decode(state.pCtx, batch_view);
			if (r == 0) // Success
			{
				cursor_pos += batch_view.n_tokens;
			}
			else if (r < 0) // Error
			{
				onComplete(InternalError::DecodeError, "llama_decode returned error");
				return;
			}
			else if (r > 0) // Insufficient cache size (possibly due to fragmentation)
			{
				DebugPrintLn(">> REBUILDING CONTEXT");
				if (!RebuildKVCache())
				{
					onComplete(InternalError::DecodeError, "llama_decode returned error");
					return;
				}
				_bCtxReallocateNextTurn = true; // Also free up more memory
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
			llm_util::process(partial, str_token, bWait, bHalt, stop_reason);
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

							if (_session.IsGroupChat() && is_bot(responderRole))
							{
								int32_t bot_index = get_bot_index(responderRole);
								SequenceId new_sequence = llm_util::sequence_from_index(bot_index);
								if (!new_sequence.IsEmpty() && new_sequence != current_sequence)
								{
									int32_t prev_sequence_index = current_sequence_index;
									current_sequence = new_sequence;
									current_sequence_indices = llm_util::get_sequence_indices(current_sequence, n_seq_max);
									current_sequence_index = current_sequence_indices[0];

									// Swap response over
									llama_kv_self_seq_cp(state.pCtx, prev_sequence_index, current_sequence_index, pre_response_pos, cursor_pos);
									//									llama_kv_self_seq_rm(state.pCtx, prev_sequence_index, pre_response_pos, cursor_pos); //! wrong?
									cache.BatchSetSequences(pre_response_pos, cursor_pos - pre_response_pos, current_sequence);

									DebugPrintLn(std::format(">> Sequence -> {}", current_sequence_index));
								}
							}
							else if (args.flags.IsSet(GenerateFlag::SwapPersonas))
								SwapPersona(responderRole);
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
				subMessageId = CreateUUID();
				PushSignal(LLMStatusSignal::CompletedMessage);
			}
		}

		// Print to console
		printf("%s", str_token.c_str());
		assert(cursor_pos < ctx_size);

		// Add sampled token to batch
		cache.BatchAddSingle(sampled_token, current_sequence_indices, cursor_pos);

		// prepare the next batch with the sampled token
		if (!next_token)
			break; // TODO: Carry over?
	}

	// Perf
	auto endTime = std::chrono::steady_clock::now();
	if (!sampled_tokens.empty())
	{
		double duration = toD(std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count());
		_tokensPerSec.store(toD(sampled_tokens.size()) / (duration / 1000.0));
	}

	// Remove full response from cache (will be reinserted on next turn)
	_contextState.ClearTokensBelow(pre_response_pos);
	//	llm_util::erase_bottom(_contextState.pCtx, _contextState.num_sequences, pre_response_pos);
	//	llm_util::clear_batch_from(batch, pre_response_pos);
	//	batch.n_tokens = pre_response_pos;
	cursor_pos = pre_response_pos;
	_contextState.previous_sequence_index = current_sequence_index;

	for (auto& block : blocks)
	{
		if (block.offset >= pre_response_pos)
			block.flags.Unset(ContextBlockFlag::Cached);
	}

	llm_util::sanitize_response(response);

	if (response.length() > 0)
	{
		if (isContinuation)
		{
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
				.tokens = sampled_tokens,
				.flags {}, // uncached
				.sequenceId { Sequence::Shared },
				.responseId = responseId,
				});
		}
	}
	else
	{
		DebugPrint("(Empty response)");
	}


#if FALSE
	std::map<string, string> variables;
	_stateVars.UpdateValues(stateReport, variables);

	// Report state variable changes
	if (!variables.empty() && _options.IsSet(LLMOption::ReportStateChanges))
	{
		string varText;
		varText.reserve(512);
		for (auto kvp : variables)
			varText = varText + std::format("{} = {}\n", kvp.first, kvp.second);
		varText = string_util::rtrim(varText);

		_resultQueue.push(MessagePiece {
			responseId = CreateUUID(),
			subMessageId = CreateUUID(),
			identifier = "",
			text = varText,
			role = Role::System,
			msgType = MessageType::SystemMessage,
			isComplete = true,
			});
	}
#endif

	DebugPrintLn();
	DebugPrintLn(std::format("END OF GENERATION (stopped on:{}) [{}]", stop_reason.c_str(), sampled_tokens.size()));

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
	default:
		return false; // Undefined
	}

	return true;
}

void LLMInstance::__ProcessTaskQueue(std::stop_token thread_stop, __GenerationCompleteCallback onComplete)
{
	InternalError error = InternalError::NoError;
	string response;
	bool bWaiting = false;

	while (true)
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
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
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

		if (_session.IsGroupChat() && !_options.IsSet(LLMOption::UseMultipleSequences))
			generateArgs.flags = generateArgs.flags | GenerateFlag::SwapPersonas;

		// Generate response
		if (_options.IsSet(LLMOption::RandomizeMessageCount))
		{
			static std::uniform_int_distribution<int> numMessages(1, 3);
			LockAndDo([&]() {
				if (generateArgs.maxMessages <= 0) // Randomize number of messages
					generateArgs.maxMessages = numMessages(_modelState.rng); //! Hmm..
			}, _stateMutex);
		}
		else if (!_options.IsSet(LLMOption::LimitMessages))
			generateArgs.maxMessages = 0;

		if (generateArgs.responseId.empty())
			generateArgs.responseId = CreateUUID();
		if (generateArgs.subMessageId.empty())
			generateArgs.subMessageId = CreateUUID();

		_activeResponseIds.insert(generateArgs.responseId);

		bWaiting = true;
		__PrepareGeneration(prepareArgs);
		__Generate(thread_stop, generateArgs,
			[this, &bWaiting, &error, &response](InternalError result, string msg) {
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

	_readyState.store(ReadyState::Generating);
	PushSignal(LLMStatusSignal::GenerationStarted);

	_workerThread = std::make_unique<std::jthread>(std::jthread(std::bind_front(&LLMInstance::__ProcessTaskQueue, this),
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
	LockAndDo([this]() {
		ClearQueue(_resultQueue);
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

		ClearQueue(_tasks);
		return true;
	}, _taskMutex);
}

bool LLMInstance::SendMessage(string message)
{
	if (string_util::empty_or_whitespace(message))
		return false;

	return EnqueueTask(LLMTask {
		.type = LLMTaskType::SendMessage,
		.input = message,
		});
}

bool LLMInstance::PushMessage(Role role, string message, MessageType msgType, bool visible, int ttl)
{
	if (string_util::empty_or_whitespace(message))
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

bool LLMInstance::__SendMessage(string message, PrepareArguments& prepareArgs, GenerateArguments& generateArgs)
{
	if (string_util::empty_or_whitespace(message))
		return false;

	__PushMessage(Role::User, message, MessageType::Undefined, true, 0);

	prepareArgs = PrepareArguments {
		.responder = Role::Undefined,
		.isContinuation = false,
		.time = 1,
	};

	GenerateFlags flags { GenerateFlag::Generate };
	if (_narratorCooldown <= 0)
		flags.Set(GenerateFlag::AllowNarrator);

	generateArgs = GenerateArguments {
		.role = Role::Undefined,
		.msgType = MessageType::Undefined,
		.flags = flags,
	};
	generateArgs.history = GetHistory(Constants::Embedding::Depth);
	return true;
}

bool LLMInstance::__PushMessage(Role role, string message, MessageType msgType, bool visible, int ttl)
{
	if (string_util::empty_or_whitespace(message))
		return false;

	// Process
	string identifier = _options.IsSet(LLMOption::UseCharacterIds) ? "@" + _session.GetIdentifierOf(role) : _session.GetNameOf(role);
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

	LockAndDo([&]() {
		_contextState.AppendBlock(ContextBlock {
			.role = role,
			.name = identifier,
			.content = content,
			.tokens {},
			.flags {},
			.sequenceId { Sequence::Shared }, //! @seq
			.offset = _contextState.GetBlockAppendOffset(),
			.ttl = ttl > 0 ? ttl + 1 : 0,
			.responseId = responseId,
			});
	}, _stateMutex);

	LockAndDo([&]() {
		_activeResponseIds.insert(responseId);
		if (visible)
		{
			// Add message to result queue
			for (auto subMsg : subMessages)
			{
				string subMessageId = CreateUUID();
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
		.time = 1,
	};

	string responder = _options.IsSet(LLMOption::UseCharacterIds) ? "@" + _session.GetIdentifierOf(role) : _session.GetNameOf(role);

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

bool LLMInstance::GreetUser()
{
	if (auto text = ReadTextFile("./resources/prompting/prompt_greeting.txt"))
	{
		string greetingInstruction = _session.ApplyNames(text.value());
		PushMessage(Role::Director, Direction(greetingInstruction), MessageType::Direction, false, 1);
		Instigate(Role::Narrator, MessageType::Narration, 1);
		Instigate(Role::Undefined, MessageType::Dialogue, 3);
		return true;
	}
	return false;
}

bool LLMInstance::Instruct(string instructions)
{
	if (auto text = ReadTextFile("./resources/prompting/prompt_formatting_director.txt"))
	{
		string prompt = text.value();
		prompt = _session.ApplyNames(prompt);
		PushMessage(Role::System, prompt, MessageType::SystemMessage, false, 1);
	}

	PushMessage(Role::Director, Direction(instructions), MessageType::Direction, false, 4);
	Instigate(Role::Undefined, MessageType::Undefined, 3);
	return true;
}

std::vector<RemovedMessage> LLMInstance::RemoveMessages(int numMessages, bool rewindTime)
{
	if (!CanGenerate() || numMessages < 1)
		return {};

	std::scoped_lock lock(_stateMutex); //! hmm...
	return impl_RemoveMessages(numMessages, rewindTime);
}

std::vector<RemovedMessage> LLMInstance::impl_RemoveMessages(int numMessages, bool rewindTime)
{
	int32_t numRemovals = toI(std::min(toSZ(numMessages), _contextState.GetBlocks().size()));
	size_t newSize = toSZ(std::max(toI(_contextState.GetBlocks().size()) - numMessages, 0));
	int32_t& cursor_pos = _contextState.cursor_pos;
	auto& blocks = _contextState.GetBlocks();

	// Rewind time
	if (rewindTime)
	{
		for (auto& block : blocks)
		{
			if (block.ttl > 0)
				block.ttl += numRemovals;
		}

		if (_narratorCooldown > 0)
			_narratorCooldown = std::min(_narratorCooldown + numRemovals, _narratorCooldownDuration);
	}

	if (newSize > 0)
	{
		const auto& newLastBlock = blocks[newSize - 1uz];
		if (newLastBlock.is_cached())
			cursor_pos = std::min(cursor_pos, newLastBlock.offset + newLastBlock.length());
		else
			cursor_pos = std::min(cursor_pos, newLastBlock.offset);
	}
	else
	{
		cursor_pos = _contextState.chat_begin_pos;
	}

	// Update batch
//	_contextState.GetBatch().n_tokens = cursor_pos;

	// Clear kv cache
	llm_util::erase_bottom(_contextState.GetModel().pCtx, _contextState.GetModel().num_sequences, cursor_pos);

	// Return removed ids
	std::vector<RemovedMessage> removedIds;
	removedIds.reserve(_contextState.GetBlocks().size() - (size_t)newSize);
	for (size_t i = (size_t)newSize; i < _contextState.GetBlocks().size(); ++i)
	{
		auto const& block = blocks[i];
		removedIds.push_back(RemovedMessage {
			block.responseId,
			block.content,
			block.role,
			});
	}

	blocks.resize((size_t)newSize);
	return removedIds;
}

std::vector<RemovedMessage> LLMInstance::RollbackUserMessage()
{
	if (!CanGenerate())
		return {};

	std::scoped_lock lock(_stateMutex);

	auto& blocks = _contextState.GetBlocks();
	for (int i = (int32_t)blocks.size() - 1; i >= 0; --i)
	{
		if (blocks[i].role == Role::User)
			return impl_RemoveMessages((int32_t)blocks.size() - i, true);
	}
	return {};
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

	LockAndDo([&]() {
		if (!_statusSignals.empty())
		{
			status.signal = _statusSignals.front();
			_statusSignals.pop();
		}
	}, _statusMutex);
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

void LLMInstance::DumpSequence(int32_t seq_id) const
{
#if _DEBUG
	llm_util::dump_batch_text(_contextState, seq_id, std::format("prompt_text_{}.txt", seq_id));
	llm_util::dump_batch_tokens(_contextState, seq_id, std::format("prompt_full_{}.txt", seq_id));
	llm_util::dump_kv_cache(_contextState, seq_id, std::format("kvcache_{}.txt", seq_id));
	llm_util::dump_kv_cache_cells(_contextState, "kvcache_alloc.txt");
#endif 
}

void LLMInstance::DumpContext() const
{
#if _DEBUG
	for (int32_t i = 0; i < _contextState.GetModel().num_sequences; ++i)
	{
		llm_util::dump_batch_text(_contextState, i, std::format("prompt_text_{}.txt", i));
		llm_util::dump_batch_tokens(_contextState, i, std::format("prompt_full_{}.txt", i));
		llm_util::dump_kv_cache(_contextState, i, std::format("kvcache_{}.txt", i));
	}
	llm_util::dump_kv_cache_cells(_contextState, "kvcache_alloc.txt");
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

void LLMInstance::PushSignal(LLMStatusSignal signal)
{
	std::scoped_lock _ { _statusMutex };

	if (!_statusSignals.empty() && _statusSignals.back() == signal)
		return;

	_statusSignals.push(signal);
}

std::set<string> LLMInstance::GetActiveMessages()
{
	std::scoped_lock _ { _resultMutex };
	return std::set<string>(_activeResponseIds.begin(), _activeResponseIds.end()); // Copy
}

void LLMInstance::RefreshActiveResponses()
{
	LockAndDo([&]() {
		_activeResponseIds.clear();
		for (const auto& block : _contextState.GetBlocks())
			_activeResponseIds.insert(block.responseId);
	}, _resultMutex);
}

bool LLMInstance::SwapPersona(Role role)
{
#if FALSE // Disabled
	if (!_session.IsGroupChat() || !(is_bot(role) || role == Role::Undefined))
		return false;

	if (role == _contextState.activePersona)
		return false; // No change

	ContextSequence& seq = _contextState.sequence;
	llama_batch& batch = _contextState.GetBatch();
	llama_context* pCtx = _modelState.pCtx;

	// Remove current persona
	auto& blocks = _contextState.blocks;
	auto itFind = std::find_if(blocks.begin(), blocks.end(), [](const ContextBlock& b) { return b.flags.IsSet(ContextBlockFlag::Persona); });
	if (itFind == blocks.end())
		return false; // No persona

	ContextBlock& block = *itFind;
	int32_t insertion_pos = block.offset;

	// Remove from kv cache
	int32_t shift = _contextState.GetCache().BatchRemove(block.offset, block.offset + block.length());

	_contextState.cursor_pos -= shift;
	_contextState.response_pos -= shift;
	_contextState.prepend_pos -= shift;

	_contextState.activePersona = Role::Undefined;

	// Activate next persona
	auto itFindInactive = _contextState.personas.find(role);
	if (itFindInactive != _contextState.personas.end())
	{
		auto& tokens = itFindInactive->second;
		int32_t len = toI(tokens.size());

		//! TODO: Allocate enough space

		// Shift down
		int32_t shift = _contextState.GetCache().BatchAllocate(insertion_pos, len);

		// Write persona to batch
		_contextState.GetCache().BatchWrite(tokens, Sequence::Default, insertion_pos);

		// Decode
		llama_batch batch_view = llm_util::create_batch_view(_contextState.GetBatch(), insertion_pos, len);
		if (batch_view.n_tokens > 0 && llama_decode(_modelState.pCtx, batch_view) != 0)
			return false; // Error

		block.tokens = tokens;
		block.role = role;

		_contextState.cursor_pos += len;
		_contextState.response_pos += len;
		_contextState.prepend_pos += len;

		_contextState.activePersona = role;
		return true;
	}
#endif
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

SamplerPtr LLMInstance::CompileGrammar(GrammarFlags flags)
{
	if (flags.IsEmpty())
		return nullptr;

	auto itFind = _modelState.grammars.find(flags);
	if (itFind != _modelState.grammars.end())
		return itFind->second;

	DebugPrintLn(std::format("Compiling grammar variant 0x{:X}", (int32_t)flags));
	SamplerPtr pGrammar = Grammar::compile_grammar(
		flags,
		_modelState.pVocab,
		_session.GetNameGrammar(_options.IsSet(LLMOption::UseCharacterIds), _options.IsSet(LLMOption::AllowUserResponse)),
		_stateVars.GetGrammarPattern());

	_modelState.grammars[flags] = pGrammar;
	return pGrammar;
}

bool LLMInstance::SetStateVariable(string name, string value, bool allowCreate)
{
	if (!_options.IsSet(LLMOption::StateVariables))
		return false;

	std::unique_lock<std::timed_mutex> stateLock(_stateMutex, std::defer_lock);
	if (!stateLock.try_lock_for(std::chrono::milliseconds(100)))
		return false;

	if (!_stateVars.HasValue(name) && !allowCreate)
		return false;

	_stateVars.SetValue(name, value);
	return true;
}

std::map<string, string> LLMInstance::GetStateVariables()
{
	std::map<string, string> result;

	std::unique_lock<std::timed_mutex> stateLock(_stateMutex, std::defer_lock);
	if (stateLock.try_lock_for(std::chrono::milliseconds(100)))
		result.insert(_stateVars.GetVariables().begin(), _stateVars.GetVariables().end());

	return result;
}

bool LLMInstance::RebuildKVCache()
{
	PushSignal(LLMStatusSignal::RebuildingContext);
	auto prevReadyState = _readyState.exchange(ReadyState::RebuildingContext);

	bool r = _contextState.RebuildKVCache();

	_readyState.store(prevReadyState);
	PushSignal(LLMStatusSignal::GenerationStarted);
	return r == 0;
}
