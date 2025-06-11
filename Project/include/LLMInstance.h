#pragma once

#include "llama.h"
#include "Types.h"
#include "ContextBuilder.h"
#include "Message.h"
#include <vector>
#include <functional>
#include <thread>
#include <mutex>
#include <queue>

struct LLMStatus;

typedef std::function<void(bool)> LoadModelCallback;
typedef std::function<void(LLMStatus)> StatusCallback;
typedef std::function<void(int)> LoadModelProgressCallback;

struct ModelState
{
	llama_model* pModel = nullptr;
	llama_context* pCtx = nullptr;
	llama_sampler* pSampler = nullptr;

	bool bReady = false;
	void Release();
};

struct LLMMessage {
    string content;
	std::vector<int32_t> tokens;
	int32_t ctx_pos;
};

struct ChatState
{
	bool isInitialized = false;
	std::vector<int32_t> system_tokens;
	std::vector<int32_t> assistant_tokens;
	std::vector<LLMMessage> messages;
	int32_t current_pos = 0;
	int32_t message_count = 0;
	llama_batch batch {};

	std::vector<Message> prev_messages;

	Character user;
	Character bot;
};

struct GenerationState
{
	int messageId = 0;
	MessageType msgType = MessageType::Undefined;
	std::string currName {};

	~GenerationState() = default;
};

struct LLMStatus
{
	string modelName;
	size_t allocCtxSize;
	size_t usedCtxSize;
};

struct MessagePiece
{
	int messageId;
	string name {};
	string text {};
	MessageType msgType = MessageType::Undefined;
	bool isComplete = false;
};

class LLMInstance
{
public:
	LLMInstance();
	~LLMInstance();

	void Shutdown();
	bool HasLoadedModel() const { return _atm_modelState.load().pModel != nullptr; }

	bool LoadModelAsync(string filename, LoadModelProgressCallback onProgress, LoadModelCallback onComplete);
	bool SendMessage(Role role, string message, bool generate = true);
	bool IsReady() const;
	bool IsGenerating() const;
	
	bool Resume();
	bool Halt();

	bool InitializeChat(string systemPrompt, std::vector<Message> messages);

	void SetStatusCallback(StatusCallback onStatus) { _statusCallback = onStatus; }

	bool PollResponse(MessagePiece& piece);
private:
	struct __PartialResult
	{
		string piece;
		string fullText;
	};

	typedef std::function<void(__PartialResult)> __PartialResultCallback;
	typedef std::function<void(int, string)> __GenerationCompleteCallback;
	void __Generate(string prompt, ChatState* chatState, __PartialResultCallback onPartial, __GenerationCompleteCallback onComplete);

	bool Generate(const string& prompt);
	void ReportStatus();

private:
	bool _bLoadingModel = false;
	string _modelName {};
	std::atomic<ModelState> _atm_modelState {};
	ChatState _chatState {};

	std::atomic<bool> _atm_bCancelGeneration {};
	std::atomic<bool> _atm_bGeneratingResponse {};
	std::unique_ptr<std::thread> _workerThread;

	std::mutex _resultMutex;
	std::string _generatedText;
	std::queue<MessagePiece> _resultQueue;
		
	StatusCallback _statusCallback = nullptr;
	int _messageCounter = 0;
};