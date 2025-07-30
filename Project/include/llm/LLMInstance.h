#pragma once

#include "LLMTypes.h"
#include "model/ChatSession.h"

#include <vector>
#include <set>
#include <functional>
#include <thread>
#include <mutex>
#include <queue>
#include <array>

using LoadModelCallback = std::function<void(bool)>;
using LoadModelProgressCallback = std::function<void(int)>;

enum class Grammar
{
	None				= 0,
	Default				= 1,
	StubDialogue		= 2,
	StubAction			= 3,
	StubNarration		= 4,
	ContinueDialogue	= 5,
	ContinueAction		= 6,
	ContinueNarration	= 7,
};

struct ModelState
{
	llama_model* pModel = nullptr;
	llama_context* pCtx = nullptr;
	llama_sampler* pSampler = nullptr;
	llama_sampler* pActiveGrammar = nullptr;
	std::array<llama_sampler*, 8> grammars = {};

	string modelName {};
	std::mt19937 rng {};

	void Release();

	llama_sampler* SetActiveGrammar(Grammar grammar);
};

struct ContextBlock 
{
	string responseId;
	Role role;
	string name;
    string content;
	std::vector<int32_t> tokens;
	int32_t ctx_pos;
	bool cached = false;
	int ttl = -1;

	int32_t length() const { return toI(tokens.size()); }
};

struct ContextState
{
	llama_batch batch {};			// Representation of the kv-cache (mirror)
	std::vector<int32_t> system_tokens;
	std::map<Role, std::vector<int32_t>> personas;
	std::vector<ContextBlock> blocks;
	int32_t persona_pos = 0;		// persona insertion point
	int32_t response_pos = 0;		// start of response
	int32_t prepend_pos = 0;
	int32_t blocks_pos = 0;			// chat start
	int32_t current_pos = 0;		// cursor position

	int32_t AssignBlockPositions();

	Role activePersona = Role::Undefined;
};

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

struct MessagePiece
{
	string responseId;		// response block
	string subMessageId;	// shared id for pieces of the same message type
	string identifier {};	// who said this?
	string content {};
	Role role = Role::Undefined;
	MessageType msgType = MessageType::Undefined;
	bool isComplete = false;
};

struct RemovedMessage 
{
	string responseId;
    string content;
	Role role;
};

class LLMInstance
{
public:
	LLMInstance();
	~LLMInstance();

	bool InitializeChat(ChatSession session, Messages messages);
	void Shutdown();

	bool IsLoadingModel() const { return _readyState.load() == ReadyState::LoadingModel; }
	bool HasLoadedModel() const { return _modelState.pModel != nullptr; }
	bool LoadModelAsync(string filename, LoadModelProgressCallback onProgress, LoadModelCallback onComplete);
	bool IsReady() const;
	bool IsGenerating() const;
	
	bool SendMessage(string message);
	bool PushMessage(Role role, string message, MessageType msgType = MessageType::Undefined, bool visible = true, int ttl = 0);

	bool Halt();
	bool Continue(string responseId, string subMessageId, bool extend);

	bool InstigateResponse(Responder responder, MessageType msgType, int messageCount = 0);
	bool GreetUser();
	bool Instruct(string instructions);
	bool ResetChat(int seed = -1);
	bool Reseed(uint32_t seed = 0xFFFFFFFF);
	std::vector<RemovedMessage> RemoveMessages(int numMessages = 1, bool rewindTime = true);
	std::vector<RemovedMessage> RollbackUserMessage();
	std::set<string> GetActiveMessages();
	bool RefreshKVCache();

	bool PollResponse(MessagePiece& piece);
	std::pair<LLMStatus, bool> PollStatus();

#if _DEBUG
	bool DumpContext(bool full, string filename = "prompt.txt") const;
#endif

	const ChatSession& GetSession() const { return _session; }

private:
	void ClearResponseQueue();
	bool CanGenerate() const;

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
	};

	using __LoadModelCallback = std::function<void(ModelState)>;
	void __LoadModel(string filename, __LoadModelCallback onComplete);

	using __PartialResultCallback = std::function<void(__PartialResult)>;
	using __GenerationCompleteCallback = std::function<void(InternalError, string)>;
	
	struct PrepareArguments
	{
		Responder responder = Responder::Bot;
		int time = 0;	// decrement ttl
	};
	void PrepareGeneration(PrepareArguments args);

	enum GenerateFlag { None = 0, Continuation, Instigation, };
	struct GenerateArguments
	{
		Role role = Role::Undefined;
		MessageType msgType = MessageType::Undefined;
		GenerateFlag flags = GenerateFlag::None;
		int maxMessages = 0;
		string prepend {};
		string responseId {};
		string subMessageId {};
	};
	void __Generate(std::stop_token stop, GenerateArguments, __PartialResultCallback onPartial, __GenerationCompleteCallback onComplete);
	void StartGeneration(GenerateArguments args);
	bool ActivatePersona(Role persona);

	void PushSignal(LLMStatusSignal signal);
	void RefreshActiveResponses();
	std::vector<RemovedMessage> impl_RemoveMessages(int numMessages, bool rewindTime);

private:
	enum class ReadyState { Invalid, Uninitialized, LoadingModel, ModelLoaded, Initializing, Ready, Generating };
	std::atomic<ReadyState> _readyState { ReadyState::Uninitialized };

	std::mutex _stateMutex; // Guards state variables
	ModelState _modelState;
	ContextState _contextState;

	std::mutex _resultMutex; // Guards output queue
	std::queue<MessagePiece> _resultQueue;
	std::set<string> _activeResponseIds;
		
	std::mutex _statusMutex; // Guards status reporting
	LLMStatus _lastStatus {};
	std::queue<LLMStatusSignal> _statusSignals;
	std::atomic<double> _tokensPerSec {};

	std::unique_ptr<std::jthread> _workerThread;

	ChatSession _session;
	
public:
	std::atomic<int64_t> _usedVRAM; // As reported from llama.cpp
	std::atomic<int64_t> _usedRAM; // As reported from llama.cpp
};