#pragma once

#include "Figment.h"
#include "chat/ChatTypes.h"
#include "io/XmlData.h"
#include "util/SearchIndex.h"

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

		void SetTitle(const fig::string& title);
		const fig::string& GetTitle() const noexcept { return _title; }
		void AddMessage(Message&& message) noexcept;
		void AddMessage(const Message& message) noexcept;

		const std::vector<Message>& GetMessages() const noexcept { return _messages; }
		const SearchIndex& GetSearchIndex() const noexcept { return _searchIndex; }
		void AddSearchTerm(const fig::string& term) noexcept;

	private:
		fig::string _title;
		std::vector<Message> _messages;
		SearchIndex _searchIndex;

	public:
		static auto XmlFields() noexcept
		{
			return Fields(
				Element { "Title",		&ChatLog::_title },
				Element { "Message",	&ChatLog::_messages }
					.Collection("Messages"),
				Element { "SearchIndex", &ChatLog::_searchIndex,
					[](auto& value) -> fig::string { return value.Serialize(); },
					[](auto& value) -> SearchIndex { SearchIndex s; s.Deserialize(value); return s; }
				}
			);

			static_assert(IsXmlSerializable<ChatLog>);
		}
	};
}
