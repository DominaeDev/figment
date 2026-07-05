#pragma once

#include "Figment.h"
#include "io/XmlSerialize.h"

namespace fig::chat
{
	struct ChatOptions
	{
		enum class Flag : uint32_t
		{
			UseCharacterIds = 1 << 0,
			AllowUserResponse = 1 << 1,
			LimitMessages = 1 << 2,
			RandomizeMessageCount = 1 << 3,
			GreetUser = 1 << 4,
			Embeddings = 1 << 5,
			Uncensored = 1 << 6,

			StateVariables = 1 << 7,
			ReportStateChanges = 1 << 8,
		};
		using Flags = EnumFlags<Flag>;

		static constexpr auto FlagMapping = std::array<std::pair<Flag, std::string_view>, 9> {
			std::pair { Flag::UseCharacterIds,			"use-ids" },
			std::pair { Flag::AllowUserResponse,		"allow-user-response" },
			std::pair { Flag::LimitMessages,			"limit-msgs" },
			std::pair { Flag::RandomizeMessageCount,	"random-msg-count" },
			std::pair { Flag::GreetUser,				"greet-user" },
			std::pair { Flag::Embeddings,				"embeddings" },
			std::pair { Flag::Uncensored,				"uncensored" },
			std::pair { Flag::StateVariables,			"state-vars" },
			std::pair { Flag::ReportStateChanges,		"report-state-changes" },
		};

		Flags flags;

		enum class GroupChatMode
		{
			Simple,			// All personas in context
			SwapPersonas,	// One persona in context
			SwapSequences,	// All personas in context (different sequences)
		};
		GroupChatMode groupChatMode = GroupChatMode::SwapSequences;

		static constexpr auto ModeMapping = std::array<std::pair<GroupChatMode, std::string_view>, 3> {
			std::pair { GroupChatMode::Simple,			"simple" },
			std::pair { GroupChatMode::SwapPersonas,	"swap-personas" },
			std::pair { GroupChatMode::SwapSequences,	"swap-sequences" },
		};

		static auto XmlFields()
		{
			using namespace fig::data;

			return Fields(
				Element { "Flags",		&ChatOptions::flags,
					[](auto& value) { return enum_serialize_flags(value, ChatOptions::FlagMapping); },
					[](auto& value) { return enum_deserialize_flags(value, ChatOptions::FlagMapping); }
				},
				Element { "Mode",		&ChatOptions::groupChatMode,
					[](auto& value) { return enum_serialize(value, ChatOptions::ModeMapping); },
					[](auto& value) { return enum_deserialize(value, ChatOptions::ModeMapping); }
				}
			);
		}
	};
}
