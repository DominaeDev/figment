#include "ChatScroll.h"
#include "VerticalListSizer.h"
#include "ChatMessage.h"
#include "AppState.h"
#include "LLMInstance.h"
#include "Color.h"
#include "StringUtil.h"
#include <format>

#define POLL_INTERVAL 0.1f

ChatScroll::ChatScroll(Control* pParent) : Control(pParent)
{
	auto pTopSizer = new VerticalListSizer();
	pTopSizer->SetBottomMargin(8);
	pTopSizer->SetSpacing(8);
	SetSizer(pTopSizer);
}

ChatMessage* ChatScroll::AddMessage(string name, string message, MessageType msgType)
{
	auto pMessage = new ChatMessage(this, name, message, msgType);
	_pSizer->Add(pMessage, 0, Sizer::Expand);
	return pMessage;
}

void ChatScroll::ClearMessages()
{
	for (auto pMessage : _children)
		delete pMessage;

	_pSizer->Clear();
	_children.clear();
	_messagesById.clear();
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
		MessageEntry* pEntry;
		auto itMsg = _messagesById.find(piece.subMessageId);
		if (itMsg != std::end(_messagesById))
		{
			// Append piece
			pEntry = &itMsg->second;

			if (pEntry->pChatMessage != nullptr)
				pEntry->pChatMessage->AppendMessage(piece.text, piece.isComplete);
			else if (!empty_or_whitespace(piece.text))
				pEntry->pChatMessage = AddMessage("Bot", piece.text, piece.msgType);
			else
				continue; // Skip until we receive some text
		}
		else if (empty_or_whitespace(piece.text) && piece.isComplete)
		{
			// Ignore complete empty messages
			continue;
		}
		else if (!empty_or_whitespace(piece.text))
		{
			// Create new message to hold the piece
			ChatMessage* pMessage = AddMessage("Bot", piece.text, piece.msgType);
			_messagesById[piece.subMessageId] = MessageEntry {
				piece.msgType,
				pMessage,
			};
			pEntry = &_messagesById[piece.subMessageId];
		}
		else
		{
			_messagesById[piece.subMessageId] = MessageEntry {
				piece.msgType,
				nullptr,
			};
		}

		// Clean up empty
		if (piece.isComplete && itMsg != std::end(_messagesById) && itMsg->second.pChatMessage == nullptr)
		{
			_messagesById.erase(itMsg);
			continue;
		}
	}
}
