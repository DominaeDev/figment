#pragma once

#include "Types.h"

enum class CommandType
{
	Invalid,
	Say,
	SystemMessage,
	UndoMessage,
	Reset,
};

struct Command
{
	CommandType type = CommandType::Say;
	string text;
};
