#pragma once

#include "Figment.h"
#include "chat/ChatTypes.h"
#include "data/ChatMessage.h"
#include "util/SearchIndex.h"

namespace fig::data
{
	class ChatLog : public XmlData<"ChatLog", 0>
	{
	public:
		void SetTitle(const fig::string& title);
		const fig::string& GetTitle() const noexcept { return _title; }
		void AddMessage(ChatMessage&& message) noexcept;
		void AddMessage(const ChatMessage& message) noexcept;

		const std::vector<ChatMessage>& GetMessages() const noexcept { return _messages; }
		const SearchIndex& GetSearchIndex() const noexcept { return _searchIndex; }
		void AddSearchTerm(const fig::string& term) noexcept;

	private:
		fig::string _title;
		std::vector<ChatMessage> _messages;
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
