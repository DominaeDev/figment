#pragma once

#include "Types.h"

struct llama_chat_message;

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

enum class ContextSequenceId : uint8_t
{
	None	= 0,
	Bot1	= 1 << 0,
	Bot2	= 1 << 1,
	Bot3	= 1 << 2,
	Bot4	= 1 << 3,
	Shared = Bot1 | Bot2 | Bot3 | Bot4,
};
DEFINE_ENUM_FLAGS(ContextSequenceId, uint8_t);

using ContextSequenceList = std::vector<int32_t>;

struct Message 
{
	Role role;
    string content;
    string name;
	ContextSequenceId seqs = ContextSequenceId::Shared;
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
