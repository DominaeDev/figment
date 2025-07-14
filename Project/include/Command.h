#pragma once

#include "Types.h"

enum class CommandType
{
	Invalid,
	Say,
	SystemMessage,
	PassTurn,
	InstigateDialogue,
	Narrate,
	Reset,
	Revert,
	Regenerate,
	Reseed,
};

struct Command
{
	CommandType type = CommandType::Say;
	string text;
};
