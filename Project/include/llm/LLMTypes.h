#pragma once

#include "Types.h"
#include <llama.h>

enum class Role
{
	Undefined = 0,
	System,
	Narrator,
	Director,
	User,
	Bot,
};

constexpr inline bool is_bot(Role role) { return role == Role::Bot; }
constexpr inline bool is_npc(Role role) { return role == Role::Director || role == Role::Narrator || role == Role::System; }

enum class MessageType
{
	Undefined = 0, 

	Dialogue,
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

struct Submessage
{
	MessageType msgType = MessageType::Undefined;
	string content;
};

enum class Responder
{ 
	None, 
	User, 
	Narrator, 
	Director, 
	Bot 
};
