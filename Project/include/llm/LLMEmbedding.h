#pragma once

#include "LLMTypes.h"
#include "model/ChatSession.h"

#include <vector>
#include <set>
#include <functional>

class LLMEmbedding
{
public:
	~LLMEmbedding();

	bool LoadModel(string filename);
	void Shutdown();
	bool IsReady() const;

	bool Generate(const std::vector<string>& history, std::vector<float>& out_embedding);
	bool Generate(std::string text, std::vector<float>& out_embedding);

private:
	bool __Generate(const std::vector<llama_token>& tokens, std::vector<float>& out_embedding);
	
private:
	llama_model* _pModel = nullptr;
	llama_context* _pCtx = nullptr;
	llama_batch* _pBatch = nullptr;
	size_t n_embed {};
};