#include <pch.h>
#include "data/ChatLog.h"

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
}