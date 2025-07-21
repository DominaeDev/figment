#pragma once

#include <string>
#include <vector>
#include <array>
#include <map>
#include <uuid_v4.h>

typedef std::string string;
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

inline constexpr uint8_t operator "" _u8( unsigned long long arg ) noexcept
{
    return static_cast<uint8_t>( arg );
}