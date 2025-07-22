#pragma once

#include "Types.h"
#include "model/Character.h"
#include <llama.h>
#include <vector>
#include <set>
#include <functional>
#include <thread>
#include <mutex>
#include <queue>
#include <array>

struct LLMStatus;

using LoadModelCallback = std::function<void(bool)>;
using LoadModelProgressCallback = std::function<void(int)>;

enum class Grammar {
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

	bool bReady = false;
	bool bInvalid = false;
	
	void Release();

	llama_sampler* SetActiveGrammar(Grammar grammar);
};

struct LLMMessageBlock 
{
	string responseId;
	Role role = Role::Undefined;
    string content;
	std::vector<int32_t> tokens;
	int32_t ctx_pos = 0;
	bool cached = false;
	int ttl = -1;

	size_t length() const { return tokens.size(); }
};

struct RemovedMessage 
{
	string responseId;
    string content;
	Role role;
};

struct ChatState
{
	bool isInitialized = false;
	std::vector<int32_t> system_tokens;
	std::vector<int32_t> assistant_tokens;
	std::vector<LLMMessageBlock> blocks;
	int32_t current_pos = 0;
	int32_t prepend_pos = 0;
	int32_t pre_response_pos = 0;
	int32_t message_count = 0;
	llama_batch batch {};

	Character user;
	Character bot;

	int32_t AssignBlockPositions();
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

enum class Responder { None, Continuation, User, Narrator, Director, Bot };

class LLMInstance
{
public:
	LLMInstance();
	~LLMInstance();

	bool InitializeChat(string systemPrompt, Messages messages);
	void Shutdown();

	bool HasLoadedModel() const { return _atModelState.load().pModel != nullptr; }
	bool IsLoadingModel() const { return _bLoadingModel; }
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
	LLMStatus GetStatus();

	bool DumpContext(string filename) const;
	
	string GetUserName() const;
	string GetBotName() const;

private:
	void ClearResponseQueue();
	void CancelGeneration();
	bool CanGenerate() const;

private:
	struct __PartialResult
	{
		string piece;
		string fullText;
	};

	enum class InternalError : int {
		NoError = 0,
		ContextFull = 1,
		DecodeError = 2,
		SamplerError = 3,
		GrammarError = 4,
	};

	using __PartialResultCallback = std::function<void(__PartialResult)>;
	using __GenerationCompleteCallback = std::function<void(InternalError, string)>;
	
	struct PrepareArguments
	{
		ChatState* pChatState;
		Responder responder = Responder::Bot;
		int time = 0;	// decrement ttl
	};
	void PrepareGeneration(PrepareArguments args);

	enum GenerateFlag { None = 0, Continuation, Instigation, };
	struct GenerateArguments
	{
		ChatState* pChat;
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

	void PushStatus(LLMStatusSignal signal);
private:
	bool _bLoadingModel = false;
	string _modelName {};

	std::atomic<ModelState> _atModelState {}; // todo: better than this
	ChatState _chatState {}; // todo: better than this
	std::set<string> _activeResponseIds;

	std::atomic<bool> _atbGeneratingResponse {};
	std::unique_ptr<std::jthread> _workerThread;

	std::mutex _resultMutex;
	std::queue<MessagePiece> _resultQueue;
		
	std::mutex _statusMutex;
	LLMStatus _lastStatus {};
	std::queue<LLMStatusSignal> _statusSignals;

	std::mt19937 _rng {};
};