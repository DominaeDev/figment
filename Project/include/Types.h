#pragma once

#include <string>
#include <vector>
#include <array>
#include <map>
#include <ranges>
#include <uuid_v4.h>

template<typename T>
inline constexpr int32_t toI(T x) { return static_cast<int32_t>(x); }
template<typename T>
inline constexpr int64_t toI64(T x) { return static_cast<int64_t>(x); }
template<typename T>
inline constexpr float toF(T x) { return static_cast<float>(x); }
template<typename T>
inline constexpr double toD(T x) { return static_cast<double>(x); }
template<typename T>
inline constexpr size_t toSZ(T x) { return static_cast<size_t>(x); }
template<typename T>
inline constexpr size_t castEnum(T x) { return static_cast<size_t>(x); }

inline constexpr uint8_t operator "" _u8( unsigned long long arg ) noexcept
{
    return static_cast<uint8_t>( arg );
}

inline constexpr size_t operator "" _sz( unsigned long long arg ) noexcept
{
    return static_cast<size_t>( arg );
}

template <std::ranges::range R>
constexpr auto to_vector(R&& r)
{
	using elem_t = std::decay_t<std::ranges::range_value_t<R>>;
	return std::vector<elem_t>{r.begin(), r.end()};
}

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