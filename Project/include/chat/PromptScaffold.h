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

	enum class PromptPriority : int8_t
	{
		Low = -1,
		Normal = 0,
		High = 1,
	};

	struct PromptBlockInfo
	{
		enum class Type
		{
			Static,
			Persona,
			User,
		} type {};

		int32_t order = 0;
		PromptPriority priority { PromptPriority:: Normal };
		Condition condition;
		fig::string content;
	public:
		static auto SerializeInfo();
	};

	struct PromptScaffold
	{
		fig::string name;

		std::vector<PromptBlockInfo> blocks;
		std::vector<PromptOption> options;

		fig::io::FileError LoadFromXml(const fig::path& filename);
	public:
		static auto SerializeInfo();
	};

	struct PromptBlock
	{
		size_t id;
		fig::string content;
		fig::hash hash;
		PromptBlockInfo::Type blockType;
		Role role = Role::Undefined;
	};
}
#endif