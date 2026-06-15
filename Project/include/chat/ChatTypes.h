#ifndef CHAT_TYPES_H__
#define CHAT_TYPES_H__

#include "Figment.h"

namespace fig::chat
{
	enum class Role : uint8_t
	{
		User = 0,

		Bot1 = 1,
		Bot2,
		Bot3,
		Bot4,
		Bot5,	// ?
		Bot6,	// ?
		Bot7,	// ?
		Bot8,	// ?

		System		= 0x20,
		Narrator,
		Director,

		Undefined = 0xFF,
	};

	constexpr auto RoleMapping = std::array<std::pair<Role, std::string_view>, 13> {
		std::pair { Role::User,			"user" },
		std::pair { Role::Bot1,			"bot1" },
		std::pair { Role::Bot2,			"bot2" },
		std::pair { Role::Bot3,			"bot3" },
		std::pair { Role::Bot4,			"bot4" },
		std::pair { Role::Bot5,			"bot5" },
		std::pair { Role::Bot6,			"bot6" },
		std::pair { Role::Bot7,			"bot7" },
		std::pair { Role::Bot8,			"bot8" },
		std::pair { Role::System,		"system" },
		std::pair { Role::Narrator,		"narrator" },
		std::pair { Role::Director,		"director" },
		std::pair { Role::Undefined,	"undefined" },
	};

	constexpr inline bool is_bot(Role role) { return role >= Role::Bot1 && role <= Role::Bot8; }
	constexpr inline bool is_user(Role role) { return role == Role::User; }
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
#endif