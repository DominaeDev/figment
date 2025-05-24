#pragma once

#include "Types.h"
#include <functional>
#include <thread>

struct llama_model;
struct llama_vocab;
struct llama_context;
struct llama_sampler;

typedef std::function<void(bool)> LoadModelCallback;

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
	bool HasLoadedModel() const { return _modelState.load().pModel != nullptr; }

	bool LoadModelAsync(string filename, LoadModelCallback onComplete);
	bool EnqueueMessage(string name, string message, string& outResponse);

private:
	bool IsReady() const;
	bool Generate(const string& prompt, string& outResponse);

	bool _bLoadingModel = false;
	std::atomic<ModelState> _modelState {};
	std::unique_ptr<std::thread> _loadThread;
};