#pragma once

#define DEFAULT_EMBEDDING_MODEL_LOCATION "M:\\Embedding\\all-MiniLM-L6-v2-Q6_K.gguf"
//#define DEFAULT_EMBEDDING_MODEL_LOCATION "M:\\Embedding\\gist-all-minilm-l6-v2.Q8_0.gguf"
#define NOMIC_EMBEDDING FALSE
#define EMBEDDING_DEPTH 1
#define EMBEDDING_SPLIT_SENTENCES TRUE

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

	bool Generate(std::string text, Embedding& out_embedding);
	bool Search(const Sentences& sentences, bool bUser = true, bool bBot = true);

private:
	enum class Mode
	{
		None,
		Query,
		Document,
	};

	bool __Generate(const std::vector<llama_token>& tokens, string content, Mode mode, Embedding& out_embedding);
#if _DEBUG
	void CompareSimilarity(const std::vector<float>& vec, size_t n_sentences);
#endif
private:
	llama_model* _pModel = nullptr;
	llama_context* _pCtx = nullptr;
	llama_batch* _pBatch = nullptr;
	string _modelName;
	size_t n_embed {};
};