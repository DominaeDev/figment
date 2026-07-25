#pragma once

#include "Figment.h"

namespace fig::chat
{
	enum class Role : uint8_t
	{
		User = 0,

		Bot1		= 0x01,
		Bot2		= 0x02,
		Bot3		= 0x03,
		Bot4		= 0x04,

		System		= 0x20,
		Narrator	= 0x21,
		Director	= 0x22,

		Undefined	= 0xFF,
	};

	constexpr Role MaxBot = Role::Bot4;

	consteval uint8_t MaxBots()
	{
		return static_cast<uint8_t>(MaxBot) - static_cast<uint8_t>(Role::Bot1) + 1;
	}

	constexpr auto RoleMapping = std::array<std::pair<Role, std::string_view>, 9> {
		std::pair { Role::User,			"user" },
		std::pair { Role::Bot1,			"bot1" },
		std::pair { Role::Bot2,			"bot2" },
		std::pair { Role::Bot3,			"bot3" },
		std::pair { Role::Bot4,			"bot4" },
		std::pair { Role::System,		"system" },
		std::pair { Role::Narrator,		"narrator" },
		std::pair { Role::Director,		"director" },
		std::pair { Role::Undefined,	"undefined" },
	};

	constexpr inline bool is_bot(Role role) { return role >= Role::Bot1 && role <= MaxBot; }
	constexpr inline bool is_user(Role role) { return role == Role::User; }
	constexpr inline bool is_npc(Role role) { return role == Role::Director || role == Role::Narrator || role == Role::System; }
	constexpr inline int32_t get_bot_index(Role role) { return is_bot(role) ? static_cast<int32_t>(role) - static_cast<int32_t>(Role::Bot1) : -1; }
	constexpr inline Role bot_from_index(int32_t botIndex)
	{
		constexpr int32_t first = (int32_t)Role::Bot1;
		constexpr int32_t last = (int32_t)MaxBot;
		if (first + botIndex < 0 || first + botIndex > last)
			return Role::Undefined;
		return static_cast<Role>(first + botIndex);
	}

	constexpr inline Role bot_from_index(size_t botIndex) { return bot_from_index(static_cast<int32_t>(botIndex)); }

	enum class MessageType : uint8_t
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

	constexpr auto MessageTypeMapping = std::array<std::pair<MessageType, std::string_view>, 8> {
		std::pair { MessageType::Undefined,		"undefined" },
		std::pair { MessageType::Dialogue,		"dialogue" },
		std::pair { MessageType::Action,		"action" },
		std::pair { MessageType::SystemMessage, "system" },
		std::pair { MessageType::Narration,		"narration" },
		std::pair { MessageType::Direction,		"direction" },
		std::pair { MessageType::StateReport,	"report" },
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
}
