#pragma once

#include "Types.h"

struct llama_model;
struct llama_vocab;
struct llama_context;
struct llama_sampler;

class Inference
{
public:
	static void Initialize();
	static void Shutdown();
	static bool HasLoadedModel() { return _pModel != nullptr; }

	static bool LoadModel(string filename);
	static bool SendMessage(string name, string message, string& outResponse);

private:
	static bool Generate(const string& prompt, string& outResponse);

	static llama_model* _pModel;
	static const llama_vocab* _pVocab;
	static llama_context* _pCtx;
	static llama_sampler* _pSampler;
};