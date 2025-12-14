#pragma once

#include "llm/LLMTypes.h"
#include "llm/Grammar.h"
#include <random>
#include <map>

class LLMEmbedding;

class ModelState
{
public:
	ModelPtr pModel = nullptr;
	VocabPtr pVocab = nullptr;
	ContextPtr pCtx = nullptr;
	SamplerPtr pSampler = nullptr;
	SamplerPtr pActiveGrammar = nullptr;

	std::map<GrammarFlags, SamplerPtr> grammars {};
	fig::string modelName {};
	std::mt19937 rng {};
	int32_t num_sequences {};
	int32_t ctx_size {};

	LLMEmbedding* pEmbedding = nullptr;

	void Release();
	bool HasGrammar(GrammarFlags flags) const;
	SamplerPtr SetActiveGrammar(GrammarFlags flags);
};

