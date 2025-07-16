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
	Look,
	Examine,
};

struct Command
{
	CommandType type = CommandType::UserMessage;
	string text;
};
