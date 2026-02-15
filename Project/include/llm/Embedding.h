#pragma once

#include "Types.h"

namespace fig::llm
{
	struct EmbeddingVector
	{
		fig::string modelName;
		fig::string content;
		std::vector<float> vec;

		bool LoadFromFile(fig::string filename);
		bool SaveToFile(fig::string filename) const;
	};

	class Embeddings
	{
	public:
		static void Initialize(fig::string filePath, fig::string modelName);

		static const std::vector<EmbeddingVector>& GetEmbeddings();
		static void AddEmbedding(EmbeddingVector embedding);
	private:
		static std::vector<EmbeddingVector> _embeddings;

	private:
		Embeddings() = delete;
		~Embeddings() = delete;
	};

	// TODO: remove static state
}