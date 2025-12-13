#ifndef CHAT_COMMANDS_H__
#define CHAT_COMMANDS_H__

#include "Types.h"

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
	RemoveLast,
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
	string text;
};

class ChatCommands
{
public:
	static ParsedChatCommand Parse(string text);
};

#endif