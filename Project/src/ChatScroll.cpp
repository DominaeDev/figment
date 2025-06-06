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

	bool isBot = !(msgType == MessageType::UserMessage || msgType == MessageType::SystemMessage || msgType == MessageType::Undefined);
	_pLastBotMessage = isBot ? pMessage : nullptr;
	return pMessage;
}

void ChatScroll::OnUpdate(float fDeltaTime)
{
	if (_bListening)
	{
		_fListenTimer += fDeltaTime;
		if (_fListenTimer >= POLL_INTERVAL)
		{
			_fListenTimer = 0.0f;
			Poll();
		}
	}
}

void ChatScroll::StartListening()
{
	_bListening = true;
	_messageId = 0;
	_messageType = MessageType::Undefined;
}

void ChatScroll::StopListening()
{
	_bListening = false;
}

void ChatScroll::Poll()
{
	auto pLLM = Application::GetLLM();
	if (!pLLM)
		return;

	MessagePiece piece;
	while (pLLM->PollResponse(piece))
	{
		if (!isEmptyOrWhitespace(piece.text))
		{
			if (!_pLastBotMessage || piece.messageId != _messageId)
			{
				_pLastBotMessage = AddMessage("Bot", piece.text, piece.msgType);
				_messageId = piece.messageId;
			}
			else
				_pLastBotMessage->AppendMessage(piece.text, piece.isComplete);
		}
		else if (_pLastBotMessage && piece.messageId == _messageId)
		{
			_pLastBotMessage->AppendMessage(piece.text, piece.isComplete);
		}

		if (piece.isComplete)
		{
			_messageId = 0;
			_messageType = MessageType::Undefined;
		}
	}
}
