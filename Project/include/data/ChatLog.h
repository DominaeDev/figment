#pragma once

#include "Figment.h"
#include "chat/ChatTypes.h"
#include "io/XmlData.h"

namespace fig::data
{
	class ChatLog : public XmlData<"ChatLog", 0>
	{
	public:
		struct Message
		{
			fig::uuid messageId;
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
					Attribute { "id", &Message::messageId }
						.MustExist(),
					Attribute { "from", &Message::speakerId }
						.SkipEmpty(),
					Attribute { "turn", &Message::turn }
						.MustExist(),
					Attribute { "role", &Message::role,
						[](auto& value) { return enum_serialize(value, RoleMapping); },
						[](auto& value) { return enum_deserialize(value, RoleMapping); }
					}	.SkipEmpty(),
					Attribute { "type", &Message::msgType,
						[](auto& value) { return enum_serialize(value, MessageTypeMapping); },
						[](auto& value) { return enum_deserialize(value, MessageTypeMapping); }
					}	.MustExist(),
					Element { "timestamp", &Message::timestamp }
						.SkipEmpty(),
					Text { &Message::content }
				);

				static_assert(IsXmlSerializable<Message>);
			}
		};

		fig::string title;
		std::vector<Message> messages;

		static auto XmlFields() noexcept
		{
			return Fields(
				Element { "Title",		&ChatLog::title },
				Element { "Message",	&ChatLog::messages }
					.Collection("Messages")
			);

			static_assert(IsXmlSerializable<ChatLog>);
		}
	};
}
