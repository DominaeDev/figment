#include "StaticText.h"
#include "Text.h"
#include "Color.h"
#include "Constants.h"
#include "AppState.h"
#include "Utility.h"

StaticText::StaticText(Control* pParent, string text, FontFace fontFace, double ptSize, bool bAutoSize) : ControlWithMargins(pParent),
	_bAutoSize(bAutoSize)
{
	_pFont = Fonts::GetFont(fontFace, ptSize);

	// Set text and measure
	_text = text;

	if (_pFont && bAutoSize)
	{
		int w, h;
		if (TTF_GetStringSize(_pFont, _text.c_str(), 0, &w, &h))
			SetSize(w, h);
	}

	DrawText();
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

void StaticText::InvalidateText()
{
	_bInvalidated = true;
}

void StaticText::OnUpdate(float fDeltaTime)
{
	if (_bInvalidated)
	{
		_bInvalidated = false;
		DrawText();
	}
}

void StaticText::OnRender(SDL_Renderer* pRenderer)
{
	DrawBackground(pRenderer);

	if (_pTexture)
	{
		SDL_FRect alignRect = GetAlignedRect();
		SDL_RenderTexture(pRenderer, _pTexture, NULL, &alignRect);
	}
}

void StaticText::DrawText()
{
	auto fgColor = GetForegroundColor();
	auto bgColor = GetBackgroundColor();
	ReleaseTexture();
	auto pRenderer = Application::GetRenderer();

	if (Color::IsDefined(fgColor) && _text.size() > 0)
	{
		// Opaque background: Use ClearType
		if (bgColor.a == 0xFF)
		{
			SDL_Surface* pSurface = TTF_RenderText_LCD_Wrapped(_pFont, _text.c_str(), 0, fgColor, bgColor, 0);
			if (pSurface)
			{
				_textWidth = pSurface->w;
				_textHeight = pSurface->h;
				_pTexture = SDL_CreateTextureFromSurface(pRenderer, pSurface);
				SDL_DestroySurface(pSurface);

				if (_bAutoSize)
					SetSize((float)(_textWidth + HMargin()), (float)(_textHeight + VMargin()));
			}
		}
		else
		{
			// Recreate text
			SDL_Surface* pSurface = TTF_RenderText_Blended_Wrapped(_pFont, _text.c_str(), 0, Color::White, 0);
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
				if (_bAutoSize)
					SetSize((float)(_textWidth + HMargin()), (float)(_textHeight + VMargin()));
			}
		}
	}
	else
	{
		_textWidth = 0;
		_textHeight = 0;
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
	int x = _rect.x + _marginLeft;
	int y = _rect.y + _marginTop;
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