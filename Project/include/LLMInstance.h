#pragma once

#include "Types.h"
#include "ContextBuilder.h"
#include "Message.h"
#include <functional>
#include <thread>
#include <mutex>
#include <queue>

struct llama_model;
struct llama_vocab;
struct llama_context;
struct llama_sampler;
struct LLMStatus;

typedef std::function<void(bool)> LoadModelCallback;
typedef std::function<void(LLMStatus)> StatusCallback;
typedef std::function<void(int)> LoadModelProgressCallback;

struct ModelState
{
	llama_model* pModel = nullptr;
	const llama_vocab* pVocab = nullptr;
	llama_context* pCtx = nullptr;
	llama_sampler* pSampler = nullptr;
	bool bReady = false;
	ContextBuilder* context_builder = nullptr;

	void Release();
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
	~LLMInstance();

	void Initialize();
	void Shutdown();
	bool HasLoadedModel() const { return _atm_modelState.load().pModel != nullptr; }

	bool LoadModelAsync(string filename, LoadModelProgressCallback onProgress, LoadModelCallback onComplete);
	bool SendMessage(string name, string message, bool generate = true);
	bool IsReady() const;
	bool IsGenerating() const;
	
	bool Resume();
	bool Halt();

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
	void __Generate(string prompt, __PartialResultCallback onPartial, __GenerationCompleteCallback onComplete);

	bool Generate(const string& prompt);
	void ReportStatus();

private:
	bool _bLoadingModel = false;
	string _modelName {};
	std::atomic<ModelState> _atm_modelState {};
	std::atomic<bool> _atm_bCancelGeneration {};
	std::atomic<bool> _atm_bGeneratingResponse {};
	std::unique_ptr<std::thread> _workerThread;

	std::mutex _resultMutex;
	std::string _generatedText;
	std::queue<MessagePiece> _resultQueue;
		
	StatusCallback _statusCallback = nullptr;
	int _messageCounter = 0;
};