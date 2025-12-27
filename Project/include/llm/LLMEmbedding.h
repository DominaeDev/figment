#pragma once

#include "model/ChatSession.h"
#include "llm/LLMTypes.h"
#include "llm/Embedding.h"

#include <vector>
#include <set>
#include <functional>

namespace fig::llm
{
	class LLMEmbedding
	{
	public:
		~LLMEmbedding();

		bool LoadModel(fig::string filename);
		void Shutdown();
		bool IsReady() const;

		bool Generate(fig::string text, EmbeddingVector& out_embedding);
		bool Search(const Sentences& sentences, bool bUser = true, bool bBot = true);

	private:
		enum class Mode
		{
			None,
			Query,
			Document,
		};

		bool __Generate(const std::vector<Token>& tokens, fig::string content, Mode mode, EmbeddingVector& out_embedding);
#if _DEBUG
		void CompareSimilarity(const std::vector<float>& vec, size_t n_sentences);
#endif
	private:
		ModelPtr _pModel = nullptr;
		ContextPtr _pCtx = nullptr;
		fig::string _modelName;
		size_t n_embed {};
	};
}