#pragma once

#include "Figment.h"

namespace fig::llm
{
	struct EmbeddingVector
	{
		fig::string modelName;
		fig::string content;
		std::vector<float> vec;

		bool LoadFromFile(const fig::path& filename);
		bool SaveToFile(const fig::path& filename) const;
	};

	class Embeddings
	{
	public:
		static void Initialize(const fig::path& filePath, fig::string modelName);

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