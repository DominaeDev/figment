#include "ChatMessage.h"
#include "StaticText.h"
#include "Color.h"
#include "Constants.h"
#include "Fonts.h"
#include "StringUtil.h"

ChatMessage::ChatMessage(Control* pParent, string name, string message) : Control(pParent),
	_name(name),
	_message(message)
{
	SetBackgroundColor(SDL_Color { 200, 200, 200, 255 });
	SetBorderColor(Color::Black);

	SetSize(-1, 60);
	string text = name + ": " + message;

	_pStaticText = new StaticText(this, text, FontFace::Default, Constants::ChatMessageFontSize, true);
	_pStaticText->SetPosition(10, 10);
}

void ChatMessage::OnRender(SDL_Renderer* pRenderer)
{
	DrawBackground(pRenderer);
}

void ChatMessage::SetMessage(const string& text)
{
	int w, h;
	_pStaticText->SetTextAndResize(trim(text), w, h);

	int currentHeight = GetHeight();
	if (currentHeight < h + 20)
	{
		SetSize(-1, h + 20);
		InvalidateParentLayout();
	}
}

void ChatMessage::AppendMessage(const string& text)
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