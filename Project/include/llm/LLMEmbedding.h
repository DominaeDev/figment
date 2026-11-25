#pragma once

#include "chat/ChatSession.h"
#include "llm/LLMTypes.h"
#include "llm/Embedding.h"

namespace fig::llm
{
	struct HistoryEntry
	{
		fig::chat::Role role;
		fig::string message;
	};
	using History = std::vector<HistoryEntry>;

	class LLMEmbedding
	{
	public:
		~LLMEmbedding();

		bool LoadModel(const fig::path& filename);
		void Shutdown();
		bool IsReady() const;

		bool Generate(fig::string text, EmbeddingVector& out_embedding);
		bool Search(const History& history, bool bUser = true, bool bBot = true);

	private:
		enum class Mode
		{
			None,
			Query,
			Document,
		};

		bool __Generate(const std::vector<Token>& tokens, fig::string content, Mode mode, EmbeddingVector& out_embedding);
		void CompareSimilarity(const std::vector<float>& vec, size_t n_sentences);
	private:
		ModelPtr _pModel = nullptr;
		ContextPtr _pCtx = nullptr;
		fig::string _modelName;
		size_t n_embed {};
	};
}