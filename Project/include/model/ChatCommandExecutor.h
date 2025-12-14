#ifndef CHAT_COMMAND_EXECUTOR_H__
#define CHAT_COMMAND_EXECUTOR_H__
#pragma once

#include "ChatCommands.h"

class LLMInstance;
class MainFrame;

class ChatCommandExecutor
{
public:
	struct Context
	{
		std::shared_ptr<LLMInstance> pLLM;
		MainFrame* pMainFrame;
	};

	static bool Execute(ParsedChatCommand command, Context context);
};

#endif