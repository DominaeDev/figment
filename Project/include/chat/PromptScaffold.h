#ifndef PROMPT_SCAFFOLD_H__
#define PROMPT_SCAFFOLD_H__
#pragma once

#include "Figment.h"
#include "text/Condition.h"

namespace fig::chat
{
	template <typename T>
	struct PromptOptionOf
	{
		fig::handle id;
		fig::string label;
		fig::string hint;
		T defaultValue {};
	};

	using PromptOptionToggle = PromptOptionOf<bool>;
	using PromptOptionNumber = PromptOptionOf<int32_t>;
	using PromptOptionText = PromptOptionOf<fig::string>;
	using PromptOption = std::variant<PromptOptionToggle, PromptOptionNumber, PromptOptionText>;

	struct PromptBlock
	{
		Condition condition;
		fig::string content;
		int32_t order = 0;

	public:
		static auto SerializeInfo();
	};

	struct PromptScaffold
	{
		fig::string name;

		std::vector<PromptBlock> blocks;
		std::vector<PromptOption> options;

		fig::io::FileError LoadFromXml(const fig::path& filename);

	public:
		static auto SerializeInfo();
	};
}
#endif