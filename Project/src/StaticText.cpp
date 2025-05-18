#include "StaticText.h"
#include "Text.h"
#include "Color.h"
#include "Constants.h"
#include "AppState.h"
#include "Utility.h"

StaticText::StaticText(Control* pParent, string text, FontFace fontFace, double ptSize) : Control(pParent)
{
	_pFont = Fonts::GetFont(fontFace, ptSize);

	SetText(text.c_str());
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

	_bInvalidated = true;
	InvalidateLayout();
}

void StaticText::OnUpdate(float fDeltaTime)
{
	if (_bInvalidated)
	{
		_bInvalidated = false;

		auto fgColor = GetForegroundColor();
		auto bgColor = GetBackgroundColor();
		ReleaseTexture();
		auto pRenderer = Application::GetRenderer();

		if (Color::IsDefined(fgColor) && _text.size() > 0)
		{
			// Opaque background: Use ClearType
			if (bgColor.a == 0xFF)
			{
				SDL_Surface* pSurface = TTF_RenderText_LCD(_pFont, _text.c_str(), 0, fgColor, bgColor);
				if (pSurface)
				{
					_textWidth = pSurface->w;
					_textHeight = pSurface->h;
					_pTexture = SDL_CreateTextureFromSurface(pRenderer, pSurface);
					SDL_DestroySurface(pSurface);
					SetSize((float)_textWidth, (float)_textHeight);
				}
			}
			else
			{
				// Recreate text
				SDL_Surface* pSurface = TTF_RenderText_Blended(_pFont, _text.c_str(), 0, Color::White);
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
					SetSize((float)_textWidth, (float)_textHeight);
				}
			}
		}
		else
		{
			_textWidth = 0;
			_textHeight = 0;
			SetSize(0, 0);
		}
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

void StaticText::OnParent()
{
	Control::OnParent();
	
	// Refresh texture
	SetText(_text);
}

Rect StaticText::GetAlignedRect() const
{
	int w = _textWidth;
	int h = _textHeight;
	Rect rect(_rect.x, _rect.y, toF(w), toF(h));

	if ((_alignment & HorizontalAlignment::Center) != 0)
		rect.x = _rect.x + (_rect.w - w) / 2;
	else if ((_alignment & HorizontalAlignment::Right) != 0)
		rect.x = _rect.x + _rect.w - w;
	if ((_alignment & VerticalAlignment::Middle) != 0)
		rect.y = _rect.y + (_rect.h - h) / 2;
	else if ((_alignment & VerticalAlignment::Bottom) != 0)
		rect.y = _rect.y + _rect.h - h;
	return rect;
}
