#pragma once

#include "chat/ChatTypes.h"
#include "io/XmlData.h"

namespace fig::data
{
	struct ChatMessage
	{
		fig::uuid responseId;
		fig::uuid subMessageId;
		fig::uuid speakerId;
		fig::chat::Role role;
		int32_t turn;
		int32_t subTurn;
		fig::timestamp timestamp;
		fig::chat::MessageType msgType;
		fig::string content;

		static auto XmlFields() noexcept
		{
			using namespace fig::chat;

			return Fields(
				Element { "MessageID",	&ChatMessage::responseId }
					.MustExist(),
				Element { "SubID",		&ChatMessage::subMessageId },
				Element { "SpeakerID",	&ChatMessage::speakerId }
					.SkipEmpty(),
				Element { "Turn",		&ChatMessage::turn },
				Element { "Subturn",	&ChatMessage::subTurn },
				Element { "Timestamp",	&ChatMessage::timestamp },
				Element { "Type",		&ChatMessage::msgType,
					[](auto& value) { return enum_serialize(value, MessageTypeMapping); },
					[](auto& value) { return enum_deserialize(value, MessageTypeMapping); }
				}	.MustExist(),
				Element { "Role",		&ChatMessage::role,
					[](auto& value) { return enum_serialize(value, RoleMapping); },
					[](auto& value) { return enum_deserialize(value, RoleMapping); }
				},
				Element { "Content",	&ChatMessage::content }
			);

			static_assert(IsXmlSerializable<ChatMessage>);
		}
	};
}