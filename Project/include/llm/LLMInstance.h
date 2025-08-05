#pragma once

#include "llm/LLMTypes.h"
#include "llm/LLMEmbedding.h"
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

struct LLMArguments
{
	LLMOption options {};
	ChatSession session;
	Messages messages;
};

class LLMInstance
{
public:
	LLMInstance();
	~LLMInstance();

	bool InitializeChat(LLMArguments args);
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

	bool InstigateResponse(Role role, MessageType msgType, int messageCount = 0);
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
	bool GenerateEmbedding(string text);
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
		Role responder = Role::Bot1;
		bool is_continue = false;
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
		Sentences history; // Used for embedding
	};
	void __Generate(std::stop_token stop, GenerateArguments, __PartialResultCallback onPartial, __GenerationCompleteCallback onComplete);
	void StartGeneration(GenerateArguments args);
	bool ActivatePersona(Role persona);

	void PushSignal(LLMStatusSignal signal);
	void RefreshActiveResponses();
	std::vector<RemovedMessage> impl_RemoveMessages(int numMessages, bool rewindTime);

	Sentences GetHistory(size_t depth);

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
	LLMOption _options;

	// Embedding
	std::unique_ptr<LLMEmbedding> _pEmbedding;
		
public:
	std::atomic<int64_t> usedVRAM; // As reported from llama.cpp
	std::atomic<int64_t> usedRAM; // As reported from llama.cpp
};