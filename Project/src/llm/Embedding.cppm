module;

export module Embedding;

import Types;
import Utility;

export 
{
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

}

union FloatToChar
{
	float f;
	int32_t i;
	uint8_t c[4];
	static_assert(sizeof(float) == 4);
	static_assert(sizeof(int32_t) == 4);
};

bool EmbeddingVector::LoadFromFile(string filename)
{
	std::ifstream file(filename); // Open the file
	if (!file.is_open())
	{
		std::cerr << "Error opening file: " << filename << std::endl;
		return false;
	}

	try
	{
		std::string line;
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

bool EmbeddingVector::SaveToFile(string filename) const
{
	try
	{
		string text = content;
		string_util::replace_all(text, ";", ",");
		string output;
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
		return WriteTextFile(filename, output, false);
	}
	catch (...)
	{
		return false;
	}
}

std::vector<EmbeddingVector> Embeddings::_embeddings;

void Embeddings::Initialize(string filePath, string modelName)
{
	_embeddings.clear();
	auto files = FindFilesInPath(filePath, ".txt");
	for (auto& fn : files)
	{
		EmbeddingVector embd;
		if (embd.LoadFromFile(fn) && string_util::equals(embd.modelName, modelName, true))
			_embeddings.push_back(std::move(embd));
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