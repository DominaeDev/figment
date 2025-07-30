#pragma once

#include "Types.h"
#include <llama.h>

enum class Role
{
	Undefined	= 0,
	System,
	Narrator,
	Director,
	User,
	Bot1		= 10,
	Bot2,
	Bot3,
	Bot4,
	Bot5,
	Bot6,
	Bot7,
	Bot8,
};

constexpr inline bool is_bot(Role role) { return role >= Role::Bot1 && role <= Role::Bot8; }
constexpr inline bool is_npc(Role role) { return role == Role::Director || role == Role::Narrator || role == Role::System; }
constexpr inline int32_t get_bot_index(Role role) { return is_bot(role) ? static_cast<int32_t>(role) - static_cast<int32_t>(Role::Bot1) : -1; }

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
