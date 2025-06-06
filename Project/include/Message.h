#pragma once

#include <string>

enum class MessageType
{
	Undefined = -1, 

	Dialogue = 0,
	QuotedDialogue,
	Action,
	Narration,
	Thought,

	UserMessage,
	SystemMessage,
};

extern std::string FormatMessage(std::string message, std::string actorName);