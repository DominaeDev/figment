#include "StatusBar.h"
#include "StaticText.h"
#include "Constants.h"
#include "Fonts.h"

StatusBar::StatusBar(Control* pParent) : Control(pParent)
{
	SetSize(pParent->GetWidth(), 24);

	SetForegroundColor(SDL_Color { 220,220,220, SDL_ALPHA_OPAQUE });
	SetBackgroundColor(SDL_Color { 40, 40, 40, SDL_ALPHA_OPAQUE });

	_pStatusText0 = new StaticText(this, "", FontFace::Default, Constants::StatusBarFontSize, false);
	_pStatusText0->SetPosition(8, 0);
	AddChild(_pStatusText0);
}

void StatusBar::SetMessage(string message)
{
	_pStatusText0->SetText(message);
}
