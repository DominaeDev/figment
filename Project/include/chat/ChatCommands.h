#ifndef CHAT_COMMANDS_H__
#define CHAT_COMMANDS_H__

#include "Figment.h"

namespace fig::chat
{
	enum class ChatCommand
	{
		Invalid,
		UserMessage,
		SystemMessage,
		PassTurn,
		InstigateDialogue,
		InstigateAction,
		Impersonate,
		Narrate,
		Instruct,
		Reset,
		Erase,
		RollbackUserMessage,
		RedoResponse,
		Reseed,
		Look,
		Examine,
		GenerateEmbedding,
		NewStateVariable,
		SetStateVariable,
	};

	struct ParsedChatCommand
	{
		ChatCommand command = ChatCommand::UserMessage;
		fig::string text;
	};

	class ChatCommands
	{
	public:
		static ParsedChatCommand Parse(fig::string text);
	};
}
#endif