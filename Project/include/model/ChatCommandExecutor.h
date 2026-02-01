#ifndef CHAT_COMMAND_EXECUTOR_H__
#define CHAT_COMMAND_EXECUTOR_H__
#pragma once

#include "ChatCommands.h"

namespace fig::gui
{
	class ChatFrame;
}

namespace fig::llm
{
	class LLMInstance;
}

class ChatCommandExecutor
{
public:
	struct Context
	{
		std::shared_ptr<fig::llm::LLMInstance> pLLM;
		fig::gui::ChatFrame* pChatFrame;
	};

	static bool Execute(ParsedChatCommand command, Context context);
};

#endif