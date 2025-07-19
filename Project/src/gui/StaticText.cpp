#include "gui/StaticText.h"
#include "gui/Text.h"
#include "gui/Color.h"
#include "model/AppState.h"
#include "util/Utility.h"
#include "Constants.h"
#include <algorithm>

StaticText::StaticText(Control* pParent, string text, FontFace fontFace, double ptSize, bool bAutoSize) : ControlWithMargins(pParent),
	_bAutoSize(bAutoSize)
{
	_pFont = Fonts::GetFont(fontFace, ptSize);

	// Set text and measure
	_text = text;
	InvalidateText();

/*	if (_pFont && bAutoSize)
	{
		int w, h;
		if (TTF_GetStringSize(_pFont, _text.c_str(), 0, &w, &h))
			SetSize(w, h);
	}

	DrawText(); */
}

StaticText::~StaticText()
{
	ReleaseTexture();
}

void StaticText::ReleaseTexture()
{
	if (_pTexture)
	{
		SDL_DestroyTexture(_pTexture);
		_pTexture = nullptr;
	}
}

void StaticText::SetText(string text)
{
	_text = text;

	InvalidateText();
	InvalidateLayout();
}

void StaticText::SetTextAndResize(string text, int& newWidth, int& newHeight)
{
	_text = text;
	_bInvalidated = false;
	DrawText(newWidth, newHeight);
	SetSize(toF(newWidth), toF(newHeight));
}

void StaticText::InvalidateText()
{
	_bInvalidated = true;
}

void StaticText::OnUpdate(float fDeltaTime)
{
	if (_bInvalidated)
	{
		_bInvalidated = false;
		int tmp;
		DrawText(tmp, tmp);
	}
}

void StaticText::OnRender(SDL_Renderer* pRenderer)
{
	auto bgColor = GetBackgroundColor();
	if (Color::IsDefined(bgColor) && bgColor.a != 0)
		DrawBackground(pRenderer);

	if (_pTexture)
	{
		SDL_FRect alignRect = GetAlignedRect();
		SDL_RenderTexture(pRenderer, _pTexture, NULL, &alignRect);
	}
}

void StaticText::DrawText(int& newWidth, int& newHeight)
{
	auto fgColor = GetForegroundColor();
	auto bgColor = GetBackgroundColor();
	ReleaseTexture();
	auto pRenderer = Application::GetRenderer();

	int maxWidth = std::max(toI(_maxSize.x), 0);

	if (Color::IsDefined(fgColor) && _text.size() > 0)
	{
		// Opaque background: Use ClearType
		if (bgColor.a == 0xFF)
		{
			SDL_Surface* pSurface = TTF_RenderText_LCD_Wrapped(_pFont, _text.c_str(), 0, fgColor, bgColor, maxWidth);
			if (pSurface)
			{
				_textWidth = pSurface->w;
				_textHeight = pSurface->h;
				_pTexture = SDL_CreateTextureFromSurface(pRenderer, pSurface);
				SDL_DestroySurface(pSurface);

				newWidth = _textWidth + HMargin();
				newHeight = _textHeight + VMargin();
				if (_bAutoSize)
					SetSize(toF(newWidth), toF(newHeight));
			}
		}
		else if (bgColor.a == 0) // Transparent background
		{
			SDL_Surface* pSurface = TTF_RenderText_Blended_Wrapped(_pFont, _text.c_str(), 0, fgColor, maxWidth);
			if (pSurface)
			{
				_textWidth = pSurface->w;
				_textHeight = pSurface->h;
				_pTexture = SDL_CreateTextureFromSurface(pRenderer, pSurface);
				SDL_DestroySurface(pSurface);

				newWidth = _textWidth + HMargin();
				newHeight = _textHeight + VMargin();
				if (_bAutoSize)
					SetSize(toF(newWidth), toF(newHeight));
			}
		}
		else
		{
			// Recreate text
			SDL_Surface* pSurface = TTF_RenderText_Blended_Wrapped(_pFont, _text.c_str(), 0, Color::White, maxWidth);
			if (pSurface)
			{
				_textWidth = pSurface->w;
				_textHeight = pSurface->h;

				// Color text
				SDL_BlendMode mode;
				SDL_GetSurfaceBlendMode(pSurface, &mode);
				SDL_Surface* pColorSurface = SDL_CreateSurface(_textWidth, _textHeight, pSurface->format);
				SDL_FillSurfaceRect(pColorSurface, NULL, SDL_MapSurfaceRGBA(pColorSurface, fgColor.r, fgColor.g, fgColor.b, fgColor.a));
				SDL_SetSurfaceBlendMode(pColorSurface, SDL_BLENDMODE_MOD);
				SDL_SetSurfaceBlendMode(pSurface, SDL_BLENDMODE_NONE);
				SDL_BlitSurface(pColorSurface, NULL, pSurface, NULL);

				_pTexture = SDL_CreateTextureFromSurface(pRenderer, pSurface);
				SDL_SetTextureBlendMode(_pTexture, SDL_BLENDMODE_BLEND);

				SDL_DestroySurface(pColorSurface);
				SDL_DestroySurface(pSurface);

				newWidth = _textWidth + HMargin();
				newHeight = _textHeight + VMargin();
				if (_bAutoSize)
					SetSize(toF(newWidth), toF(newHeight));
			}
		}
	}
	else
	{
		_textWidth = 0;
		_textHeight = 0;
		newWidth = 0;
		newHeight = 0;
		if (_bAutoSize)
			SetSize(0, 0);
	}
}

void StaticText::OnParent()
{
	Control::OnParent();
	
	// Refresh texture
	SetText(_text);
}

Rect StaticText::GetAlignedRect() const
{
	int x = toI(_rect.x + _marginLeft);
	int y = toI(_rect.y + _marginTop);
	int w = _textWidth;
	int h = _textHeight;
	Rect rect(toF(x), toF(y), toF(w), toF(h));

	if ((_alignment & HorizontalAlignment::Center) != 0)
		rect.x = x + (_rect.w - w) / 2;
	else if ((_alignment & HorizontalAlignment::Right) != 0)
		rect.x = x + _rect.w - w;
	if ((_alignment & VerticalAlignment::Middle) != 0)
		rect.y = y + (_rect.h - h) / 2;
	else if ((_alignment & VerticalAlignment::Bottom) != 0)
		rect.y = y + _rect.h - h;
	return rect;
}

void StaticText::SetForegroundColor(SDL_Color color) 
{ 
	_foregroundColor = color;
	InvalidateText();
}

void StaticText::SetBackgroundColor(SDL_Color color) 
{ 
	_backgroundColor = color; 
	InvalidateText();
}