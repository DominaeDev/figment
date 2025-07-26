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

	size_t length() const { return tokens.size(); }
};

struct ContextState
{
	std::vector<int32_t> system_tokens;
	std::array<std::vector<int32_t>,4> personas;
	std::vector<ContextBlock> blocks;
	int32_t current_pos = 0;
	int32_t prepend_pos = 0;
	int32_t pre_response_pos = 0;
	int32_t blocks_begin = 0;	// post-system prompt
	int32_t floor_pos = 0;		// bottom
	llama_batch batch {};

	int32_t AssignBlockPositions();
	void SwapInPersona(size_t index);

	// Testing
	std::vector<int32_t> secret_tokens;
	int32_t secret_pos = 0;
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
	bool bReady = false;
	bool bInvalid = false;
	double tokensPerSec = 0.0;
	LLMStatusSignal signal;
};

struct MessagePiece
{
	string responseId;		// response block
	string subMessageId;	// shared id for pieces of the same message type
	string name {};
	string text {};
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

	bool PollResponse(MessagePiece& piece);
	std::pair<LLMStatus, bool> PollStatus();

	bool DumpContext(bool full) const;

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
};