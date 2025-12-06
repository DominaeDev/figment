#ifndef MODEL_STATE_H__
#define MODEL_STATE_H__

#include "LLMTypes.h"
#include "Grammar.h"
#include <random>

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

#endif