#ifndef EMBEDDING_H__
#define EMBEDDING_H__
#pragma once

#include "Types.h"

struct EmbeddingVector
{
	string modelName;
	string content;
	std::vector<float> vec;

	bool LoadFromFile(string filename);
	bool SaveToFile(string filename) const;
};

class Embeddings
{
public:
	static void Initialize(string filePath, string modelName);

	static const std::vector<EmbeddingVector>& GetEmbeddings();
	static void AddEmbedding(EmbeddingVector embedding);
private:
	static std::vector<EmbeddingVector> _embeddings;

private:
	Embeddings() = delete;
	~Embeddings() = delete;
};
#endif