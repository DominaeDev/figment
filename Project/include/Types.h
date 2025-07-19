#pragma once

#include <string>
#include <vector>
#include <map>
#include <uuid_v4.h>

typedef std::string string;

struct llama_chat_message;

enum class Role
{
	System,
	Narrator,
	Director,
	User,
	Bot,
};

enum class MessageType
{
	Undefined = -1, 

	Dialogue = 0,
	Action,
	Thought,

	SystemMessage,
	Narration,
	Direction,
};

struct Message 
{
	Role role;
    string content;
    string name;
};
using Messages = std::vector<Message>;
