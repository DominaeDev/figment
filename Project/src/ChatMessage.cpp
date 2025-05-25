#include "ChatMessage.h"
#include "StaticText.h"
#include "Color.h"
#include "Constants.h"
#include "Fonts.h"
#include "AppState.h"
#include "LLMInstance.h"

ChatMessage::ChatMessage(Control* pParent, string name, string message) : Control(pParent),
	_name(name),
	_message(message)
{
	SetBackgroundColor(SDL_Color { 200, 200, 200, 255 });
	SetBorderColor(Color::Black);
	SetSize(-1, 80);

	string text = name + ": " + message;

	_pStaticText = new StaticText(this, text, FontFace::Default, Constants::DefaultFontSize);
	_pStaticText->SetPosition(10, 10);
}

#define POLL_INTERVAL 0.1f

void ChatMessage::OnUpdate(float fDeltaTime)
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

void ChatMessage::OnRender(SDL_Renderer* pRenderer)
{
	DrawBackground(pRenderer);
}

void ChatMessage::StartListening()
{
	_bListening = true;
}

void ChatMessage::StopListening()
{
	_bListening = false;
}

void ChatMessage::Poll()
{
	auto pLLM = Application::GetLLM();
	if (!pLLM)
		return;

	string piece;
	if (pLLM->TryGetResponse(piece))
		AppendText(piece);
}

void ChatMessage::AppendText(const string& text)
{
	_pStaticText->SetText(_pStaticText->GetText() + text);
}