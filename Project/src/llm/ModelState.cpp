#include <pch.h>
#include "llm/ModelState.h"
#include "llm/LlamaApi.h"
#include "llm/LLMEmbedding.h"

#include <list>
#include <cassert>

namespace fig::llm
{
	void ModelState::Release()
	{
		if (pSampler)
		{
			SetActiveGrammar(GrammarFlags::None); // Detach grammar (if any)
			llama::free(pSampler);
		}

		for (auto& kvp : grammars)
			llama::free(kvp.second);
		grammars.clear();

		if (pCtx)
		{
			llama::ctx_clear(pCtx);
			llama::free(pCtx);
		}

		if (pModel)
			llama::free(pModel);

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

	SamplerPtr ModelState::SetActiveGrammar(GrammarFlags flags)
	{
		SamplerPtr pSelectedGrammar = nullptr;
		if (flags)
		{
			auto itFind = grammars.find(flags);
			if (itFind != grammars.end())
				pSelectedGrammar = itFind->second;
		}

		SamplerPtr pChain = pSampler;
		if (pSelectedGrammar != nullptr && pSelectedGrammar == llama_sampler_chain_get(pChain, 0))
			return pSelectedGrammar; // No swap

		std::list<SamplerPtr> samplers;
		int32_t n = llama_sampler_chain_n(pChain);

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
}