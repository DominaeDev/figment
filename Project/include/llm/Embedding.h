#pragma once

#include "Types.h"

struct Embedding
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

	static const std::vector<Embedding>& GetEmbeddings();
	static void AddEmbedding(Embedding embedding);
private:
	static std::vector<Embedding> _embeddings;

private:
	Embeddings() = delete;
	~Embeddings() = delete;
};