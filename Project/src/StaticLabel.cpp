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

	ClearBackground(pRenderer);

	if (_pTexture)
	{
		SDL_FRect alignRect = GetAlignedRect();
		SDL_RenderTexture(pRenderer, _pTexture, NULL, &alignRect);
	}
}

void StaticLabel::OnParent()
{
	Control::OnParent();
	
	// Refresh texture
	SetText(_pText);
}

SDL_FRect StaticLabel::GetAlignedRect() const
{
	if (_pSurface == nullptr)
		return _rect; // Error
	
	int w = _pSurface->w;
	int h = _pSurface->h;
	SDL_Rect rect { _rect.x, _rect.y, w, h };

	if ((_alignment & HorizontalAlignment::Center) != 0)
		rect.x = _rect.x + (_rect.w - w) / 2;
	else if ((_alignment & HorizontalAlignment::Right) != 0)
		rect.x = _rect.x + _rect.w - w;
	if ((_alignment & VerticalAlignment::Middle) != 0)
		rect.y = _rect.y + (_rect.h - h) / 2;
	else if ((_alignment & VerticalAlignment::Bottom) != 0)
		rect.y = _rect.y + _rect.h - h;

	return SDL_FRect { 
		static_cast<float>(rect.x), 
		static_cast<float>(rect.y), 
		static_cast<float>(rect.w), 
		static_cast<float>(rect.h) 
	};
}
