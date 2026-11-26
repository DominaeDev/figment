#include <pch.h>
#include "data/ChatLog.h"

using namespace fig::chat;

namespace fig::data
{
	void ChatLog::SetTitle(const fig::string& title)
	{
		_title = title;
	}

	void ChatLog::AddMessage(ChatMessage&& message) noexcept
	{
		_searchIndex.AddTerm(message.content);
		_messages.emplace_back(std::move(message));
	}

	void ChatLog::AddMessage(const ChatMessage& message) noexcept
	{
		_searchIndex.AddTerm(message.content);
		_messages.push_back(message);
	}

	void ChatLog::AddSearchTerm(const fig::string& term) noexcept
	{
		_searchIndex.AddTerm(term);
	}

	bool ChatLog::OnLoadFromXml(XmlReaderElement node)
	{
		for (auto& message : _messages)
		{
			if (message.role == Role::Undefined)
			{
				switch (message.msgType)
				{
				case MessageType::Narration:
					message.role = Role::Narrator;
					break;
				case MessageType::Direction:
					message.role = Role::Director;
					break;
				case MessageType::SystemMessage:
					message.role = Role::System;
					break;
				}
			}
		}

		return true;
	}
}