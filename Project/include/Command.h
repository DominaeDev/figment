#pragma once

#include "Types.h"

enum class CommandType
{
	Invalid,
	UserMessage,
	SystemMessage,
	PassTurn,
	InstigateDialogue,
	InstigateAction,
	Impersonate,
	Narrate,
	Guide,
	Reset,
	Revert,
	RollbackUserMessage,
	RedoResponse,
	Reseed,
};

struct Command
{
	CommandType type = CommandType::UserMessage;
	string text;
};
