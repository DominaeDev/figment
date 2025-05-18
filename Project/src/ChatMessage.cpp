#include "ChatMessage.h"
#include "StaticText.h"
#include "Color.h"
#include "Constants.h"
#include "Fonts.h"

ChatMessage::ChatMessage(Control* pParent, string name, string message) : Control(pParent),
	_name(name),
	_message(message)
{
	SetBackgroundColor(SDL_Color { 200, 200, 200, 255 });
	SetBorderColor(Color::Black);
	SetSize(-1, 80);

	string text = name + ": " + message;

	_pStaticText = new StaticText(this, text, FontFace::Default, Constants::DefaultFontSize);
	_pStaticText->SetSize(GetSize());
	_pStaticText->SetPosition(10, 10);
}

void ChatMessage::OnRender(SDL_Renderer* pRenderer)
{
	DrawBackground(pRenderer);
}