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
			int16_t subindex;
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
						.MustExist(),
					Attribute { "turn", &Message::turn }
						.MustExist(),

					Attribute { "role", &Message::role,
						[](auto& value) { return enum_serialize(value, RoleMapping); },
						[](auto& value) { return enum_deserialize(value, RoleMapping); }
					}	.MustExist(),
					Attribute { "type", &Message::msgType,
						[](auto& value) { return enum_serialize(value, MessageTypeMapping); },
						[](auto& value) { return enum_deserialize(value, MessageTypeMapping); }
					}	.MustExist(),
					Element { "timestamp", &Message::timestamp },
					Text { &Message::content }
						.MustExist()
				);

				static_assert(IsXmlSerializable<Message>);
			}
		};

		fig::uuid assetId;
		std::vector<Message> messages;

		static auto XmlFields() noexcept
		{
			return Fields(
				Attribute { "id", &ChatLog::assetId }
					.MustExist(),
				Element { "Message", &ChatLog::messages }
					.Collection("Messages")
					.MustExist()
			);

			static_assert(IsXmlSerializable<ChatLog>);
		}
	};
}
