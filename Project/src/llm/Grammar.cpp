#include <pch.h>
#include "llm/Grammar.h"
#include "io/FileUtility.h"
#include "text/TextEvaluator.h"
#include <cassert>

using namespace fig::io;

namespace fig::llm
{
	static bool _evaluate(fig::string& text, size_t pos_begin, const std::set<fig::string>& flags)
	{
		size_t pos_next = text.find("{{", pos_begin + 2);
		size_t pos_end = text.find("}}", pos_begin + 2);
		if (pos_end == fig::npos)
			return false;

		if (pos_next != fig::npos && pos_next < pos_end)
		{
			_evaluate(text, pos_next, flags); // Evaluate inner
			return _evaluate(text, pos_begin, flags); // Re-evaluate outer
		}

		size_t pos_q = text.find('?', pos_begin + 2);
		if (pos_q == fig::npos || pos_q > pos_end)
			return false;

		fig::string condition = text.substr(pos_begin + 2, pos_q - pos_begin - 2);
		auto expected_flags = split(condition, '|', true);
		bool result = true;
		for (auto& flag : expected_flags)
		{
			bool bTrue = true;
			if (flag[0] == '!')
			{
				flag = flag.substr(1);
				bTrue = false;
			}
			if ((flags.find(flag) != flags.end()) != bTrue)
			{
				result = false;
				break;
			}
		}

		if (result)
		{
			text.erase(pos_end, 2); // }}
			text.erase(pos_begin, pos_q - pos_begin + 1); // {{...?
		}
		else
			text.erase(pos_begin, pos_end - pos_begin + 2);
		return true;
	}

	SamplerPtr Grammar::compile_grammar(const fig::string& grammar, GrammarFlags grammarFlags, VocabPtr pVocab, fig::string names, fig::string stateVars)
	{
		if (grammar.empty())
			return nullptr;

		Context ctx;
		ctx.SetFlag("default", grammarFlags.IsSet(GrammarFlag::Default));
		ctx.SetFlag("stub", grammarFlags.IsSet(GrammarFlag::Stub));
		ctx.SetFlag("continue", grammarFlags.IsSet(GrammarFlag::Continue));
		ctx.SetFlag("talk", grammarFlags.IsSet(GrammarFlag::Talk));
		ctx.SetFlag("act", grammarFlags.IsSet(GrammarFlag::Act));
		ctx.SetFlag("narrate", grammarFlags.IsSet(GrammarFlag::Narrate));
		ctx.SetFlag("enable_narrator", grammarFlags.IsSet(GrammarFlag::EnableNarrator));
		ctx.SetFlag("enable_state", grammarFlags.IsSet(GrammarFlag::EnableState));
		ctx.SetValue("names", names);
		ctx.SetValue("state_vars", stateVars);

		auto compiled_grammar = eval_text(grammar, ctx, TextEvaluationOptions::None);

		auto pGrammar = llama_sampler_init_grammar(pVocab, compiled_grammar.c_str(), "root");
		assert(pGrammar);
		return pGrammar;
	}
}