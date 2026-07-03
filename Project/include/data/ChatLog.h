#ifndef CHAT_LOG_H__
#define CHAT_LOG_H__
#pragma once

#include "Figment.h"
#include "chat/ChatTypes.h"
#include "io/IXmlSerializable.h"

namespace fig::data
{
	class ChatLog : public IXmlSerializable<"ChatLog", 0>
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

			static auto SerializeInfo() noexcept
			{
				using namespace fig::chat;

				return Fields(
					AsAttribute { "id", &Message::messageId }
						.MustExist(),
					AsAttribute { "from", &Message::speakerId }
						.MustExist(),
					AsAttribute { "turn", &Message::turn }
						.MustExist(),

					AsAttribute { "role", &Message::role,
						[](auto& value) { return enum_serialize(value, RoleMapping); },
						[](auto& value) { return enum_deserialize(value, RoleMapping); }
					}	.MustExist(),
					AsAttribute { "type", &Message::msgType,
						[](auto& value) { return enum_serialize(value, MessageTypeMapping); },
						[](auto& value) { return enum_deserialize(value, MessageTypeMapping); }
					}	.MustExist(),
					AsElement { "timestamp", &Message::timestamp },
					AsText { &Message::content }
						.MustExist()
				);

				static_assert(XmlSerializable<Message>);
			}
		};

		fig::uuid assetId;
		std::vector<Message> messages;

		static auto SerializeInfo() noexcept
		{
			return Fields(
				AsAttribute { "id", &ChatLog::assetId }
					.MustExist(),
				AsElement { "Message", &ChatLog::messages }
					.Collection("Messages")
					.MustExist()
			);

			static_assert(XmlSerializable<ChatLog>);
		}
	};
}


#endif