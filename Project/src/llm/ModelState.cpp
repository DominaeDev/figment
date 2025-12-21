#include <pch.h>
#include "llm/ModelState.h"
#include "llm/LLMEmbedding.h"

#include <list>
#include <cassert>

using namespace fig::llm;

void ModelState::Release()
{
	if (pSampler)
	{
		SetActiveGrammar(GrammarFlags::None); // Detach grammar (if any)
		llama_sampler_free(pSampler);
	}

	for (auto& kvp : grammars)
		llama_sampler_free(kvp.second);
	grammars.clear();

	if (pCtx)
	{
		llama_kv_self_clear(pCtx);
		llama_free(pCtx);
	}

	if (pModel)
		llama_model_free(pModel);

	if (pEmbedding)
	{
		pEmbedding->Shutdown();
		pEmbedding = nullptr;
	}

	pSampler = nullptr;
	pActiveGrammar = nullptr;
	pCtx = nullptr;
	pModel = nullptr;
	pVocab = nullptr;
}

llama_sampler* ModelState::SetActiveGrammar(GrammarFlags flags)
{
	llama_sampler* pSelectedGrammar = nullptr;
	if (flags)
	{
		auto itFind = grammars.find(flags);
		if (itFind != grammars.end())
			pSelectedGrammar = itFind->second;
	}

	llama_sampler* pChain = pSampler;
	if (pSelectedGrammar != nullptr && pSelectedGrammar == llama_sampler_chain_get(pChain, 0))
		return pSelectedGrammar; // No swap

	std::list<llama_sampler*> samplers;
	int32_t n = llama_sampler_chain_n(pChain);
	assert(n <= 5);

	for (int32_t i = n - 1; i >= 0; --i)
	{
		samplers.push_front(llama_sampler_chain_get(pChain, i));
		llama_sampler_chain_remove(pChain, i);
	}

	samplers.remove(pActiveGrammar);
	pActiveGrammar = nullptr;

	if (pSelectedGrammar)
	{
		samplers.push_front(pSelectedGrammar);
		pActiveGrammar = pSelectedGrammar;
	}

	for (auto sampler : samplers)
		llama_sampler_chain_add(pChain, sampler);
	return pActiveGrammar;
}

bool ModelState::HasGrammar(GrammarFlags flags) const
{
	return grammars.find(flags) != grammars.end();
}