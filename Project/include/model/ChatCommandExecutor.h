#ifndef CHAT_COMMAND_EXECUTOR_H__
#define CHAT_COMMAND_EXECUTOR_H__
#pragma once

#include "ChatCommands.h"

class MainFrame;
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
		MainFrame* pMainFrame;
	};

	static bool Execute(ParsedChatCommand command, Context context);
};

#endif