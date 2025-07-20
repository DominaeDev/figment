#include "gui/ChatScroll.h"
#include "gui/VerticalListSizer.h"
#include "gui/ChatMessage.h"
#include "gui/Color.h"
#include "model/AppState.h"
#include "llm/LLMInstance.h"
#include "llm/LLMUtility.h"
#include "util/StringUtility.h"
#include <format>
#include <set>

#define POLL_INTERVAL 0.1f

ChatScroll::ChatScroll(Control* pParent) : Control(pParent)
{
	auto pTopSizer = new VerticalListSizer();
	pTopSizer->SetBottomMargin(8);
	pTopSizer->SetSpacing(8);
	SetSizer(pTopSizer);
}

ChatMessage* ChatScroll::AddMessage(string name, Role role, MessageType msgType, string message, bool complete)
{
	if (msgType == MessageType::Narration)
		role = Role::Narrator;

	bool bShowAvatar = (role == Role::Bot || role == Role::User);
	auto itLast = std::find_if(std::crbegin(_messages), std::crend(_messages), [](const MessageEntry& entry) {
		return entry.pChatMessage != nullptr;
	});
	
	Role lastRole = Role::Undefined;
	if (itLast != std::crend(_messages))
	{
		if ((*itLast).msgType == MessageType::Narration)
			lastRole = Role::Narrator;
		else
			lastRole = (*itLast).role;
	}
	bShowAvatar &= role != lastRole;
	bool bShowName = role != lastRole;
	if (msgType == MessageType::Narration)
		name = llm_util::name_from_role(Role::Narrator);

	auto pMessage = new ChatMessage(this, bShowName ? name : "", role, msgType, bShowAvatar, bShowName);
	pMessage->SetMessage(message, complete);
	_pSizer->Add(pMessage, 0, Sizer::Expand);
	return pMessage;
}

int ChatScroll::RemoveMessages(std::vector<string> ids)
{
	int removed = 0;
	std::set<string> removedIds;
	for (int i = (int32_t)_messages.size() - 1; i >= 0; --i)
	{
		MessageEntry& entry = _messages[i];
		if (std::find(std::cbegin(ids), std::cend(ids), entry.responseId) == std::cend(ids))
			continue;

		if (entry.pChatMessage)
		{
			_pSizer->Remove(entry.pChatMessage);
			RemoveChild(entry.pChatMessage);
			delete entry.pChatMessage;
		}

		removedIds.insert(entry.subMessageId);
		_messages.erase(std::begin(_messages) + (ptrdiff_t)i);
		++removed;
	}

	for (auto id : removedIds)
		_messagesById.erase(id);

	InvalidateLayout();
	return removed;
}

void ChatScroll::ClearMessages()
{
	for (auto pMessage : _children)
		delete pMessage;

	_pSizer->Clear();
	_children.clear();
	_messages.clear();
	_messagesById.clear();
}

std::tuple<std::string, std::string> ChatScroll::GetLastMessage() const
{
	if (_messages.empty())
		return {};
	auto& message = _messages[_messages.size() - 1];
	return std::make_tuple(message.responseId, message.subMessageId);
}

void ChatScroll::OnUpdate(float fDeltaTime)
{
	_fPollTimer += fDeltaTime;
	if (_fPollTimer >= POLL_INTERVAL)
	{
		_fPollTimer = 0.0f;
		Poll();
	}
}

void ChatScroll::EnablePolling(bool bEnable)
{
	_bPolling = bEnable;
}

void ChatScroll::Poll()
{
	auto pLLM = Application::GetLLM();
	if (!pLLM)
		return;

	MessagePiece piece;
	while (pLLM->PollResponse(piece))
	{
		auto itMsg = _messagesById.find(piece.subMessageId);
		if (itMsg != std::end(_messagesById))
		{
			// Append piece
			MessageEntry* pEntry = itMsg->second;

			if (pEntry->pChatMessage != nullptr)
				pEntry->pChatMessage->AppendMessage(piece.text, piece.isComplete);
			else if (!string_util::empty_or_whitespace(piece.text))
			{
				(*itMsg).second->role = piece.role;
				(*itMsg).second->msgType = piece.msgType;
				pEntry->pChatMessage = AddMessage(piece.name, piece.role, piece.msgType, piece.text, piece.isComplete);
			}
			else
				continue; // Skip until we receive some text
		}
		else if (string_util::empty_or_whitespace(piece.text) && piece.isComplete)
		{
			// Ignore complete empty messages
			continue;
		}
		else if (!string_util::empty_or_whitespace(piece.text))
		{
			// Create new message to hold the piece
			ChatMessage* pMessage = AddMessage(piece.name, piece.role, piece.msgType, piece.text, piece.isComplete);
			_messages.push_back(MessageEntry {
				piece.role,
				piece.responseId,
				piece.subMessageId,
				piece.msgType,
				pMessage,
			});
			_messagesById[piece.subMessageId] = &_messages.back();
		}
		else // Empty or whitespace
		{
			_messages.push_back(MessageEntry {
				piece.role,
				piece.responseId,
				piece.subMessageId,
				piece.msgType,
				nullptr,
			});
			_messagesById[piece.subMessageId] = &_messages.back();
		}

		// Clean up empty
		if (piece.isComplete && itMsg != std::end(_messagesById) && itMsg->second->pChatMessage == nullptr)
		{
			_messagesById.erase(itMsg);
			for (int i = (int32_t)_messages.size() - 1; i >= 0; --i)
			{
				if (_messages[i].subMessageId == itMsg->first)
					_messages.erase(std::begin(_messages) + (ptrdiff_t)i);
			}
			continue;
		}
	}
}
