#pragma once

#include "chat/PromptScaffold.h"

namespace fig::chat
{
	class ChatStaging;

	class PromptBuilder
	{
	public:
		static std::vector<PromptBlock> GetStagingBlocks(ChatStaging& staging) noexcept;
	};
}
