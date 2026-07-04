#pragma once

#include "ChatCommands.h"

namespace fig::gui
{
	class ChatScreen;
}

namespace fig::llm
{
	class LLMInstance;
}

namespace fig::chat
{
	class ChatCommandExecutor
	{
	public:
		struct Context
		{
			std::shared_ptr<fig::llm::LLMInstance> pLLM;
			fig::gui::ChatScreen* pChatFrame;
		};

		static bool Execute(ParsedChatCommand command, Context context);
	};
}
