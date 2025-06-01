#include "ChatScroll.h"
#include "VerticalListSizer.h"
#include "ChatMessage.h"
#include "AppState.h"
#include "LLMInstance.h"
#include "Color.h"

#define POLL_INTERVAL 0.1f

ChatScroll::ChatScroll(Control* pParent) : Control(pParent)
{
	auto pTopSizer = new VerticalListSizer();
	pTopSizer->SetBottomMargin(8);
	pTopSizer->SetSpacing(8);
	SetSizer(pTopSizer);
}

ChatMessage* ChatScroll::AddMessage(string name, string message, bool isUser)
{
	auto pLLM = Application::GetLLM();
	
	SDL_Color color = isUser ? (pLLM && pLLM->IsReady() ? Color::UserMessageBackground : Color::DarkGray) : Color::BotMessageBackground;

	auto pMessage = new ChatMessage(this, name, message, color);
	_pSizer->Add(pMessage, 0, Sizer::Expand);

	_pLastBotMessage = isUser ? nullptr : pMessage;
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
}

void ChatScroll::StopListening()
{
	_bListening = false;
}

void ChatScroll::Poll()
{
	if (!_pLastBotMessage)
		return;

	auto pLLM = Application::GetLLM();
	if (!pLLM)
		return;

	string piece;
	if (pLLM->TryGetResponse(piece))
		_pLastBotMessage->AppendMessage(piece);
}
