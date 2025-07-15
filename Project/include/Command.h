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
	Narrate,
	Reset,
	Revert,
	Regenerate,
	Reseed,
};

struct Command
{
	CommandType type = CommandType::UserMessage;
	string text;
};
