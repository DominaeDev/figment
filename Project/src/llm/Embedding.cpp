#include "llm/Embedding.h"
#include "util/Common.h"
#include "util/StringUtility.h"
#include "util/FileUtility.h"
#include <format>
#include <iostream>
#include <fstream>

using namespace fig::string_util;
using namespace fig::file_util;

union FloatToChar 
{
	float f;
	int32_t i;
	uint8_t c[4];
	static_assert(sizeof(float) == 4);
	static_assert(sizeof(int32_t) == 4);
};

bool EmbeddingVector::LoadFromFile(fig::string filename)
{
	std::ifstream file(filename); // Open the file
    if (!file.is_open()) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return false;
    }

	try
	{
		fig::string line;
		if (!std::getline(file, line, ';'))
			return false;
		modelName = line;
		if (!std::getline(file, line, ';'))
			return false;
		content = line;

		vec.clear();
		vec.reserve(512);
		while (std::getline(file, line, ';'))
		{
			FloatToChar fc;
			fc.c[0] = static_cast<uint8_t>(std::stoi(line.substr(0, 2), nullptr, 16));
			fc.c[1] = static_cast<uint8_t>(std::stoi(line.substr(2, 2), nullptr, 16));
			fc.c[2] = static_cast<uint8_t>(std::stoi(line.substr(4, 2), nullptr, 16));
			fc.c[3] = static_cast<uint8_t>(std::stoi(line.substr(6, 2), nullptr, 16));
			vec.push_back(fc.f);
		}
	}
	catch (std::exception e)
	{
		file.close(); // Close the file
		return false;
	}

	file.close(); // Close the file
	return !modelName.empty() && !content.empty() && !vec.empty();
}

bool EmbeddingVector::SaveToFile(fig::string filename) const
{
	try
	{
		fig::string text = content;
		replace_all(text, ";", ",");
		fig::string output;
		output.reserve(8192);
		output += modelName + ";";
		output += text + ";";
		for (size_t i = 0; i < vec.size(); ++i)
		{
			if (i > 0)
				output += ";";
			FloatToChar fc;
			fc.f = vec[i];
			for (size_t j = 0; j < sizeof(float); ++j)
				output += std::format("{:02X}", fc.c[j]);
		}
		return WriteTextFile(filename, output, false) == FileError::NoError;
	}
	catch (...)
	{
		return false;
	}
}

std::vector<EmbeddingVector> Embeddings::_embeddings;

void Embeddings::Initialize(fig::string filePath, fig::string modelName)
{
	_embeddings.clear();
	auto files = FindFilesInPath(filePath, ".txt");
	if (files.has_value())
	{
		for (auto& fn : files.value())
		{
			EmbeddingVector embd;
			if (embd.LoadFromFile(fn) && equals(embd.modelName, modelName, true))
				_embeddings.push_back(std::move(embd));
		}
	}
}

const std::vector<EmbeddingVector>& Embeddings::GetEmbeddings()
{
	return _embeddings;
}

void Embeddings::AddEmbedding(EmbeddingVector embedding)
{
	_embeddings.push_back(embedding);
}