#include "ChatMessage.h"
#include "StaticText.h"
#include "NineGridBackgroundRenderer.h"
#include "Color.h"
#include "Constants.h"
#include "Fonts.h"
#include "StringUtil.h"
#include "Utility.h"
#include "CustomRenderer.h"

ChatMessage::ChatMessage(Control* pParent, string name, string message, MessageType msgType) : Control(pParent),
	_name(name),
	_message(message),
	_messageType(msgType)
{
	SDL_Color bgColor;
	switch (msgType)
	{
	case MessageType::Dialogue:
		bgColor = Color::BotMessageBackground;
		break;
	case MessageType::Action:
		bgColor = Color::BotMessageBackground;
		break;
	case MessageType::UserMessage:
		bgColor = Color::UserMessageBackground;
		break;
	default:
		bgColor = Color::NarrationBackground;
		break;
	}

	SetBackgroundRenderer(new NineGridBackgroundRenderer(10.0f, bgColor, Color::AddRGB(bgColor, 0.15f) ));
	SetBackgroundColor(bgColor);

	SetSize(-1, 60);

	_pStaticText = new StaticText(this, message, FontFace::Default, Constants::ChatMessageFontSize, true);
	_pStaticText->SetPosition(10, 10);
}

void ChatMessage::OnRender(SDL_Renderer* pRenderer)
{
	DrawBackground(pRenderer);
}

void ChatMessage::SetMessage(const string& text)
{
	int w, h;
	_message = trim(text);
	_pStaticText->SetTextAndResize(_message, w, h);

	// Resize
	int currentHeight = toI(GetHeight());
	if (currentHeight < h + 20)
	{
		SetSize(GetWidth(), toF(h + 20));
		InvalidateParentLayout(true);
	}
}

void ChatMessage::AppendMessage(const string& text, bool lastPiece)
{
	SetMessage(_pStaticText->GetText() + text);
}

void ChatMessage::OnSize()
{
	Control::OnSize();

	if (_bIgnoreEvent)
		return;

	if (_pStaticText)
		_pStaticText->SetMaxSize(GetWidth() - 20, -1);
}