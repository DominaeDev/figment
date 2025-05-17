#include "StaticLabel.h"
#include "Text.h"
#include "Color.h"
#include "Constants.h"
#include "AppState.h"

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
}

void StaticLabel::SetText(const char* text)
{
	_pText = text;
	_bInvalidated = true;
	InvalidateLayout();
}

void StaticLabel::OnUpdate(float fDeltaTime)
{
	if (_bInvalidated)
	{
		_bInvalidated = false;

		auto fgColor = GetForegroundColor();
		auto bgColor = GetBackgroundColor();
		ReleaseTexture();
		auto pRenderer = Application::GetRenderer();

		if (Color::IsDefined(fgColor) && _pText)
		{
			// Opaque background: Use ClearType
			if (bgColor.a == 0xFF)
			{
				SDL_Surface* pSurface = TTF_RenderText_LCD(_pFont, _pText, 0, fgColor, bgColor);
				if (pSurface)
				{
					_textWidth = pSurface->w;
					_textHeight = pSurface->h;
					_pTexture = SDL_CreateTextureFromSurface(pRenderer, pSurface);
					SDL_DestroySurface(pSurface);
					SetSize(_textWidth, _textHeight);
				}
			}
			else
			{
				// Recreate text
				SDL_Surface* pSurface = TTF_RenderText_Blended(_pFont, _pText, 0, Color::White);
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
					SetSize(_textWidth, _textHeight);
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

void StaticLabel::OnRender(SDL_Renderer* pRenderer)
{
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
	int w = _textWidth;
	int h = _textHeight;
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
