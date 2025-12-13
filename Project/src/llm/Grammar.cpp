#include "llm/Grammar.h"
#include "util/Common.h"
#include "util/StringUtility.h"
#include "util/FileUtility.h"
#include <set>
#include <cassert>

static bool _evaluate(string& text, size_t pos_begin, const std::set<string>& flags)
{
	size_t pos_next = text.find("{{", pos_begin + 2);
	size_t pos_end = text.find("}}", pos_begin + 2);
	if (pos_end == string::npos)
		return false;

	if (pos_next != string::npos && pos_next < pos_end)
	{
		_evaluate(text, pos_next, flags); // Evaluate inner
		return _evaluate(text, pos_begin, flags); // Re-evaluate outer
	}

	size_t pos_q = text.find('?', pos_begin + 2);
	if (pos_q == string::npos || pos_q > pos_end)
		return false;

	string condition = text.substr(pos_begin + 2, pos_q - pos_begin - 2);
	auto expected_flags = string_util::split(condition, '|', true);
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

SamplerPtr Grammar::compile_grammar(GrammarFlags grammarFlags, VocabPtr pVocab, string names, string stateVars)
{
	string grammar = file_util::ReadTextFile("./resources/grammar/formatting_grammar.gbnf").value_or("");
	if (grammar.size() == 0)
		return nullptr;

	string_util::replace_all(grammar, "{{_NAMES_}}", names);
	string_util::replace_all(grammar, "{{_STATE_VARS_}}", stateVars);

	std::set<string> flags;
	if (grammarFlags.IsSet(GrammarFlag::Default))
		flags.insert("default");
	if (grammarFlags.IsSet(GrammarFlag::Stub))
		flags.insert("stub");
	if (grammarFlags.IsSet(GrammarFlag::Continue))
		flags.insert("continue");
	if (grammarFlags.IsSet(GrammarFlag::Talk))
		flags.insert("talk");
	if (grammarFlags.IsSet(GrammarFlag::Act))
		flags.insert("act");
	if (grammarFlags.IsSet(GrammarFlag::Narrate))
		flags.insert("narrate");
	if (grammarFlags.IsSet(GrammarFlag::EnableNarrator))
		flags.insert("enable-narrator");
	if (grammarFlags.IsSet(GrammarFlag::EnableState))
		flags.insert("enable-state");

	size_t pos = grammar.find("{{", 0);
	while (pos != string::npos && pos < grammar.size())
	{
		if (_evaluate(grammar, pos, flags))
		{
			assert(grammar[pos] != '{');
			pos = grammar.find("{{", pos);
		}
		else
			pos = grammar.find("{{", pos + 2);
	}

	auto pGrammar = llama_sampler_init_grammar(pVocab, grammar.c_str(), "root");
	assert(pGrammar);
	return pGrammar;
}