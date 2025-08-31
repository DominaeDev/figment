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

struct Command
{
	CommandType type = CommandType::UserMessage;
	string text;
};
