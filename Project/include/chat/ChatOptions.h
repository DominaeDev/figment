#ifndef CHAT_OPTIONS_H__
#define CHAT_OPTIONS_H__

#pragma once

#include "Figment.h"

namespace fig::chat
{
	struct ChatOptions
	{
		enum class Flag : uint32_t
		{
			UseCharacterIds = 1 << 0,
			AllowUserResponse = 1 << 1,
			LimitMessages = 1 << 2,
			RandomizeMessageCount = 1 << 3,
			GreetUser = 1 << 4,
			Embeddings = 1 << 5,
			Uncensored = 1 << 6,

			StateVariables = 1 << 7,
			ReportStateChanges = 1 << 8,
		};
		using Flags = EnumFlags<Flag>;

		Flags flags;

		enum class GroupChatMode
		{
			Simple,			// All personas in context
			SwapPersonas,	// One persona in context
			SwapSequences,	// All personas in context (different sequences)
		};
		GroupChatMode groupChatMode = GroupChatMode::SwapSequences;
	};
}
#endif