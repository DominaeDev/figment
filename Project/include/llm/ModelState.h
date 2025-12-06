#pragma once

#include "LLMTypes.h"
#include "Grammar.h"

class ModelState
{
public:
	ModelPtr pModel = nullptr;
	VocabPtr pVocab = nullptr;
	ContextPtr pCtx = nullptr;
	SamplerPtr pSampler = nullptr;
	SamplerPtr pActiveGrammar = nullptr;

	std::map<GrammarFlags, SamplerPtr> grammars {};
	string modelName {};
	std::mt19937 rng {};
	int32_t num_sequences {};
	int32_t ctx_size {};

	void Release();
	bool HasGrammar(GrammarFlags flags) const;
	SamplerPtr SetActiveGrammar(GrammarFlags flags);
};

