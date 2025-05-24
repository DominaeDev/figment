#pragma once

#include "Types.h"
#include <functional>
#include <thread>
#include <mutex>

struct llama_model;
struct llama_vocab;
struct llama_context;
struct llama_sampler;

typedef std::function<void(bool)> LoadModelCallback;
typedef std::function<void(int)> LoadModelProgressCallback;

struct ModelState
{
	llama_model* pModel = nullptr;
	const llama_vocab* pVocab = nullptr;
	llama_context* pCtx = nullptr;
	llama_sampler* pSampler = nullptr;
	bool bReady = false;

	void Release();
};

class LLMInstance
{
public:
	void Initialize();
	void Shutdown();
	bool HasLoadedModel() const { return _atm_modelState.load().pModel != nullptr; }

	bool LoadModelAsync(string filename, LoadModelProgressCallback onProgress, LoadModelCallback onComplete);
	bool SendMessage(string name, string message);
	bool Stop();

	bool TryGetResponse(string& result);

private:
	struct __PartialResult
	{
		string response;
		string piece;
	};

	typedef std::function<void(__PartialResult)> PartialResultCallback;
	typedef std::function<void(int, string)> GenerationCompleteCallback;
	void __Generate(const string& prompt, PartialResultCallback onPartial, GenerationCompleteCallback onComplete);

	bool IsReady() const;
	bool Generate(const string& prompt);

	bool _bLoadingModel = false;

	std::atomic<ModelState> _atm_modelState {};
	std::atomic<bool> _atm_bCancelGeneration {};
	std::atomic<bool> _atm_bGeneratingResponse {};

	std::unique_ptr<std::thread> _workerThread;

	std::mutex _mutex_generatedText;
	std::string _generatedText;
	std::string _lastResponse;
};