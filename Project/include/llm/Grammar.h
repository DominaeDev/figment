#pragma once

#include "llm/LLMTypes.h"
namespace fig::llm
{
	enum class GrammarFlag : int32_t
	{
		None = 0,

		// Generation type
		Default = 1 << 0,
		Stub = 1 << 1,
		Continue = 1 << 2,

		// Message type
		Talk = 1 << 3,
		Act = 1 << 4,
		Narrate = 1 << 5,

		// Options
		EnableNarrator = 1 << 6,
		EnableState = 1 << 7,
		UseCharacterIds = 1 << 8,
		AllowUser = 1 << 9,
	};
	using GrammarFlags = EnumFlags<GrammarFlag>;

	namespace Grammar
	{
		SamplerPtr compile_grammar(GrammarFlags grammarFlags, VocabPtr pVocab, fig::string names, fig::string stateVars);
	}
}