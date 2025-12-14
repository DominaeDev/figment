#pragma once

#include "llm/LLMTypes.h"
#include "llm/Grammar.h"
#include <random>
#include <map>

class LLMEmbedding;

class ModelState
{
public:
	fig::ModelPtr pModel = nullptr;
	fig::VocabPtr pVocab = nullptr;
	fig::ContextPtr pCtx = nullptr;
	fig::SamplerPtr pSampler = nullptr;
	fig::SamplerPtr pActiveGrammar = nullptr;

	std::map<GrammarFlags, fig::SamplerPtr> grammars {};
	fig::string modelName {};
	std::mt19937 rng {};
	int32_t num_sequences {};
	int32_t ctx_size {};

	LLMEmbedding* pEmbedding = nullptr;

	void Release();
	bool HasGrammar(GrammarFlags flags) const;
	fig::SamplerPtr SetActiveGrammar(GrammarFlags flags);
};

