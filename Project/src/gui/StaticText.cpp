#include <pch.h>
#include "gui/StaticText.h"
#include "gui/GUICommon.h"
#include "model/AppState.h"
#include "Constants.h"
#include <algorithm>

using namespace fig::gui::util;

namespace fig::gui
{
	constexpr Color DropShadowColor { 0x00, 0x00, 0x00, 0xC0 };
	constexpr float DropShadowDistance { 1.25f };

	StaticText::StaticText(LayoutElement* pParent, fig::string text, FontFace fontFace, double ptSize, bool bAutoSize) : Control(pParent),
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
		_texture.clear();
		_shadow.clear();
	}

	void StaticText::SetText(fig::string text)
	{
		_text = text;

		InvalidateText();
		InvalidateLayout();
	}

	void StaticText::SetTextAndResize(fig::string text, float& newWidth, float& newHeight)
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

	void StaticText::OnUpdate(float fElapsed)
	{
		if (_bInvalidated)
		{
			_bInvalidated = false;
			float tmp;
			DrawText(tmp, tmp);
		}
	}

	void StaticText::OnRender(Renderer* pRenderer)
	{
		auto bgColor = GetBackgroundColor();
		if (bgColor.IsDefined() && bgColor.a != 0)
			DrawBackground(pRenderer);

		bool test = SDL_RenderClipEnabled(pRenderer);

		if (not _shadow.empty() and _bDropShadow)
		{
			Rectf alignRect = GetAlignedRect();
			alignRect.x += DropShadowDistance;
			alignRect.y += DropShadowDistance;
			SDL_RenderTexture(pRenderer, _shadow.get(), NULL, &alignRect);
		}

		if (not _texture.empty())
		{
			Rectf alignRect = GetAlignedRect();
			SDL_RenderTexture(pRenderer, _texture.get(), NULL, &alignRect);
		}
	}

	void StaticText::DrawText(float& newWidth, float& newHeight)
	{
		auto fgColor = GetForegroundColor();
		auto bgColor = GetBackgroundColor();
		ReleaseTexture();
		auto pRenderer = GetSDLRenderer();

		if (_text.empty())
		{
			newWidth = 0;
			newHeight = 0;
			return;
		}

		int maxWidth = std::max(toI(_maxSize.x), 0);
		const char* pText = _text.c_str();

		std::string altText;
		if (_bEllipsis)
		{
			altText = GetEllipsisText(_text);
			pText = altText.c_str();
		}

		if (_bDropShadow)
			DrawShadow(pText);


		if (fgColor.IsDefined())
		{
			// Opaque background: Use ClearType
			if (bgColor.a == 0xFF)
			{
				SDL_Surface* pSurface = _bWordWrap ?
					TTF_RenderText_LCD_Wrapped(_pFont, pText, 0, fgColor, bgColor, maxWidth)
					: TTF_RenderText_LCD(_pFont, pText, 0, fgColor, bgColor);
				if (pSurface)
				{
					_textWidth = pSurface->w;
					_textHeight = pSurface->h;

					_texture.reset(SDL_CreateTextureFromSurface(pRenderer, pSurface));
					SDL_DestroySurface(pSurface);

					newWidth = _textWidth + GetMarginHorizontal();
					newHeight = _textHeight + GetMarginVertical();
					if (_bAutoSize)
						SetSize(toF(newWidth), toF(newHeight));
					return;
				}
			}
			else if (bgColor.a == 0) // Transparent background
			{
				SDL_Surface* pSurface = _bWordWrap ?
					TTF_RenderText_Blended_Wrapped(_pFont, pText, 0, fgColor, maxWidth)
					: TTF_RenderText_Blended(_pFont, pText, 0, fgColor);
				if (pSurface)
				{
					_textWidth = pSurface->w;
					_textHeight = pSurface->h;
					_texture.reset(SDL_CreateTextureFromSurface(pRenderer, pSurface));
					SDL_DestroySurface(pSurface);

					newWidth = _textWidth + GetMarginHorizontal();
					newHeight = _textHeight + GetMarginVertical();
					if (_bAutoSize)
						SetSize(toF(newWidth), toF(newHeight));
					return;
				}
			}
			else
			{
				// Recreate text
				SDL_Surface* pSurface = _bWordWrap ?
					TTF_RenderText_Blended_Wrapped(_pFont, pText, 0, Colors::White, maxWidth)
					: TTF_RenderText_Blended(_pFont, pText, 0, Colors::White);
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

					_texture.reset(SDL_CreateTextureFromSurface(pRenderer, pSurface));
					SDL_SetTextureBlendMode(_texture.get(), SDL_BLENDMODE_BLEND);

					SDL_DestroySurface(pColorSurface);
					SDL_DestroySurface(pSurface);

					newWidth = _textWidth + GetMarginHorizontal();
					newHeight = _textHeight + GetMarginVertical();
					if (_bAutoSize)
						SetSize(toF(newWidth), toF(newHeight));
					return;
				}
			}
		}

		_textWidth = 0;
		_textHeight = 0;
		newWidth = 0;
		newHeight = 0;
		if (_bAutoSize)
			SetSize(0, 0);
	}

	void StaticText::DrawShadow(const char* pText)
	{
		auto pRenderer = GetSDLRenderer();
		auto fgColor = GetForegroundColor();

		int maxWidth = std::max(toI(_maxSize.x), 0);

		_shadow.clear();

		if (fgColor.IsDefined())
		{
			SDL_Surface* pSurface = _bWordWrap ?
				TTF_RenderText_Blended_Wrapped(_pFont, pText, 0, DropShadowColor, maxWidth)
				: TTF_RenderText_Blended(_pFont, pText, 0, DropShadowColor);

			if (pSurface)
			{
				_shadow.reset(SDL_CreateTextureFromSurface(pRenderer, pSurface));
				SDL_DestroySurface(pSurface);
			}
		}
	}

	void StaticText::OnParent()
	{
		Control::OnParent();

		// Refresh texture
		SetText(_text);
	}

	Rectf StaticText::GetAlignedRect() const
	{
		int x = toI(_rect.x + GetMarginLeft());
		int y = toI(_rect.y + GetMarginTop());
		int w = _textWidth;
		int h = _textHeight;
		Rectf rect(toF(x), toF(y), toF(w), toF(h));

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

	void StaticText::SetForegroundColor(Color color)
	{
		Control::SetForegroundColor(color);
		InvalidateText();
	}

	void StaticText::SetBackgroundColor(Color color)
	{
		Control::SetBackgroundColor(color);
		InvalidateText();
	}

	constexpr fig::string ellipsis(const fig::string& text, size_t length) noexcept
	{
		return text.substr(0, std::min(length, text.size())) + "\u2026";
	}

	fig::string StaticText::GetEllipsisText(const fig::string& text) const
	{
		if (text.empty())
			return "";

		int maxWidth = std::max(toI(_maxSize.x), 0);
		if (maxWidth == 0)
			return text;

		int w, h;
		if (TTF_GetStringSize(_pFont, text.c_str(), 0, &w, &h) and w <= maxWidth)
			return text;

		fig::string testString;
		testString.reserve(text.length());
		size_t pos = 1;
		for (; pos < text.length(); pos += 6)
		{
			testString = ellipsis(text, pos);
			if (TTF_GetStringSize(_pFont, testString.c_str(), 0, &w, &h) and w > maxWidth)
				break;
		}

		--pos;
		for (; pos > 0; --pos)
		{
			testString = text.substr(0, pos) + "\u2026";
			if (TTF_GetStringSize(_pFont, testString.c_str(), 0, &w, &h) and w <= maxWidth)
				return testString;
		}
		return text;
	}

	Point StaticText::MeasureText(bool bAllowEllipsis) const
	{
		if (_bEllipsis and bAllowEllipsis)
		{
			auto text = GetEllipsisText(_text);
			int w, h;
			if (TTF_GetStringSize(_pFont, text.c_str(), 0, &w, &h))
				return Point(w, h);
		}
		else
		{
			int w, h;
			if (TTF_GetStringSize(_pFont, _text.c_str(), 0, &w, &h))
				return Point(w, h);
		}
		return Point(0, 0);
	}
}