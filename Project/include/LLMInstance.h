#pragma once

#include "llama.h"
#include "Types.h"
#include "Message.h"
#include "Character.h"
#include <vector>
#include <functional>
#include <thread>
#include <mutex>
#include <queue>

struct LLMStatus;

typedef std::function<void(bool)> LoadModelCallback;
typedef std::function<void(int)> LoadModelProgressCallback;

struct ModelState
{
	llama_model* pModel = nullptr;
	llama_context* pCtx = nullptr;
	llama_sampler* pSampler = nullptr;
	llama_sampler* pGrammar = nullptr;

	bool bReady = false;
	bool bInvalid = false;
	void Release();
};

struct LLMMessageBlock {
	uuid responseId;
	Role role;
    string content;
	std::vector<int32_t> tokens;
	int32_t ctx_pos;
	bool cached = false;
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
};

struct LLMStatus
{
	string modelName;
	size_t allocCtxSize = 0;
	size_t usedCtxSize = 0;
	bool bReady = false;
	bool bInvalid = false;
};

struct MessagePiece
{
	uuid responseId;	// response block
	uuid subMessageId;	// shared id for pieces of the same message type
	string name {};
	string text {};
	MessageType msgType = MessageType::Undefined;
	bool isComplete = false;
};

struct Message 
{
	Role role;
    string content;
    string name;
};
typedef std::vector<Message> Messages;

enum class Responder { None, Narrator, User, Bot };

class LLMInstance
{
public:
	LLMInstance();
	~LLMInstance();

	bool InitializeChat(string systemPrompt, Messages messages);
	void Shutdown();

	bool HasLoadedModel() const { return _atm_modelState.load().pModel != nullptr; }
	bool IsLoadingModel() const { return _bLoadingModel; }
	bool LoadModelAsync(string filename, LoadModelProgressCallback onProgress, LoadModelCallback onComplete);
	bool IsReady() const;
	bool IsGenerating() const;
	
	bool SendMessage(Role role, string message);
	bool PushMessage(Role role, string message, MessageType msgType = MessageType::UserMessage, bool visible = true);

	bool Halt();
	bool Resume();

	bool InstigateResponse(Responder responder, MessageType msgType, int messageCount = 0);
	bool GreetUser();
	bool ResetChat(int seed = -1);
	bool Reseed(uint32_t seed = 0xFFFFFFFF);
	std::vector<uuid> RemoveMessages(int numMessages = 1);
	std::vector<uuid> RollbackUserMessage();

	bool PollResponse(MessagePiece& piece);
	LLMStatus GetStatus() const;

	bool DumpContext(string filename) const;
	
	string GetUserName() const;
	string GetBotName() const;

private:
	void ClearResponseQueue();
	void CancelWorkerThread();
	bool CanGenerate() const;

private:
	struct __PartialResult
	{
		string piece;
		string fullText;
	};

	enum InternalError : int {
		NoError = 0,
		ContextFull = 1,
		DecodeError = 2,
		SamplerError = 3,
		GrammarError = 4,
	};

	typedef std::function<void(__PartialResult)> __PartialResultCallback;
	typedef std::function<void(InternalError, string)> __GenerationCompleteCallback;
	
	struct PrepareArguments
	{
		ChatState* pChatState;
		Responder responder = Responder::Bot;
	};
	void PrepareGeneration(PrepareArguments args);

	struct GenerateArguments
	{
		ChatState* pChat;
		MessageType msgType = MessageType::Undefined;
		int maxMessages = 0;
		string prepend;
	};
	void Generate(std::stop_token stop, GenerateArguments, __PartialResultCallback onPartial, __GenerationCompleteCallback onComplete);

private:
	bool _bLoadingModel = false;
	string _modelName {};
	std::atomic<ModelState> _atm_modelState {};
	ChatState _chatState {};

	std::atomic<bool> _atm_bGeneratingResponse {};
	std::unique_ptr<std::jthread> _workerThread;

	std::mutex _resultMutex;
	std::queue<MessagePiece> _resultQueue;
		
	LLMStatus _lastStatus {};
	int _messageCounter = 0;
};