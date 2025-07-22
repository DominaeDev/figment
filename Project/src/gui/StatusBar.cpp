#include "gui/StatusBar.h"
#include "gui/StaticText.h"
#include "gui/Fonts.h"
#include "Constants.h"
#include <format>

StatusBar::StatusBar(Control* pParent) : Control(pParent)
{
	SetSize(pParent->GetWidth(), 24);

	SetForegroundColor(Color { 0x51, 0x4a, 0x2f, SDL_ALPHA_OPAQUE });
	SetBackgroundColor(Color { 0xde, 0xd9, 0xc5, SDL_ALPHA_OPAQUE });

	_pMessage = new StaticText(this, "", FontFace::Default, Constants::StatusBarFontSize, false);
	_pMessage->SetPosition(8, 1);

	_pModelInfo = new StaticText(this, "", FontFace::Default, Constants::StatusBarFontSize, false);
	_pModelInfo->SetPosition(400, 0);
	AddChild(_pMessage);
}

void StatusBar::SetMessage(string message)
{
	_pMessage->SetText(message);
}

void StatusBar::SetModelInfo(string modelName, size_t maxCtxSize, size_t usedCtxSize)
{
	if (!modelName.empty())
		_pModelInfo->SetText(std::format("Model: {} Ctx: {:d}/{:d}", modelName, usedCtxSize, maxCtxSize));
	else
		_pModelInfo->SetText("");
}
