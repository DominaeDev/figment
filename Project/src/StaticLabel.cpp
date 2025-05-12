#include "StaticLabel.h"
#include "Text.h"
#include "Utility.h"

StaticLabel::StaticLabel(const char* text, FontFace fontFace, double ptSize)
{
	_pFont = Fonts::GetFont(fontFace, ptSize);

	if (text != nullptr)
		SetText(text);
}

StaticLabel::~StaticLabel()
{
	ReleaseTexture();
}

void StaticLabel::ReleaseTexture()
{
	if (_pTexture)
	{
		SDL_DestroyTexture(_pTexture);
		_pTexture = nullptr;
	}

	if (_pSurface)
	{
		SDL_DestroySurface(_pSurface);
		_pSurface = nullptr;
	}
}

void StaticLabel::SetText(const char* text)
{
	_pText = text;
	auto fgColor = GetForegroundColor();
	auto bgColor = GetBackgroundColor();

	if (!ColorIsDefined(fgColor) || !ColorIsDefined(bgColor) || text == nullptr)
		return;

	ReleaseTexture();
	
	_pSurface = TTF_RenderText_LCD_Wrapped(_pFont, _pText, 0, fgColor, SDL_Color { bgColor.r, bgColor.g, bgColor.b, 255 }, 0);
	SetSize((float)_pSurface->w, (float)_pSurface->h);
}

void StaticLabel::OnRender(SDL_Renderer* pRenderer)
{
	if (!_pTexture && _pSurface)
		_pTexture = SDL_CreateTextureFromSurface(pRenderer, _pSurface);

	if (_pTexture)
		SDL_RenderTexture(pRenderer, _pTexture, NULL, &_rect);
}

void StaticLabel::OnParent()
{
	Control::OnParent();
	
	// Refresh texture
	SetText(_pText);
}