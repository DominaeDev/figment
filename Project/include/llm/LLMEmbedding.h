#pragma once

#include "model/ChatSession.h"
#include "llm/LLMTypes.h"
#include "llm/Embedding.h"

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

	bool Generate(const std::vector<string>& history, Embedding& out_embedding);
	bool Generate(std::string text, bool bSearch, Embedding& out_embedding);

private:
	bool __Generate(const std::vector<llama_token>& tokens, string content, Embedding& out_embedding);
#if _DEBUG
	void CompareSimilarity(const std::vector<float>& vec);
#endif
private:
	llama_model* _pModel = nullptr;
	llama_context* _pCtx = nullptr;
	llama_batch* _pBatch = nullptr;
	string _modelName;
	size_t n_embed {};
};