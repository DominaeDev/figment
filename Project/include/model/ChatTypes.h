#ifndef CHAT_TYPES_H__
#define CHAT_TYPES_H__

#include "Types.h"

enum class Role : int32_t
{
	Undefined = 0,
	System,
	Narrator,
	Director,
	User,

	Bot1 = 10,
	Bot2,
	Bot3,
	Bot4,
	Bot5,	// ?
	Bot6,	// ?
	Bot7,	// ?
	Bot8,	// ?
};

constexpr inline bool is_bot(Role role) { return role >= Role::Bot1 && role <= Role::Bot8; }
constexpr inline bool is_npc(Role role) { return role == Role::Director || role == Role::Narrator || role == Role::System; }
constexpr inline int32_t get_bot_index(Role role) { return is_bot(role) ? static_cast<int32_t>(role) - static_cast<int32_t>(Role::Bot1) : -1; }
constexpr inline Role bot_from_index(int32_t botIndex)
{
	constexpr int32_t first = (int32_t)Role::Bot1;
	constexpr int32_t last = (int32_t)Role::Bot8;
	if (first + botIndex < 0 || first + botIndex > last)
		return Role::Undefined;
	return static_cast<Role>(first + botIndex);
}

constexpr inline Role bot_from_index(size_t botIndex) { return bot_from_index(static_cast<int32_t>(botIndex)); }

enum class MessageType
{
	Undefined = 0,

	Dialogue,
	Action,
	Thought,

	SystemMessage,
	Narration,
	Direction,

	StateReport,
};

struct Message
{
	Role role;
	fig::string content;
	fig::string name;
};
using Messages = std::vector<Message>;

struct Submessage
{
	MessageType msgType = MessageType::Undefined;
	fig::string content;
};

struct Sentence
{
	Role role;
	fig::string sentence;
};
using Sentences = std::vector<Sentence>;

#endif