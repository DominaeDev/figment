#ifndef PROMPT_BUILDER_H__
#define PROMPT_BUILDER_H__
#pragma once

#include "chat/PromptScaffold.h"

namespace fig::chat
{
	class ChatStaging;
	struct PromptBlock
	{
		size_t id;
		fig::string content;
		fig::hash hash;
	};

	class PromptBuilder
	{
	public:
		static std::vector<PromptBlock> GetBlocks(const PromptScaffold& scaffold, ChatStaging& staging) noexcept;
	};
}
#endif