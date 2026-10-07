#include <pch.h>
#include "gui/StaticText.h"
#include "gui/GUICommon.h"
#include "app/AppState.h"
#include <algorithm>

namespace fig::gui
{
	constexpr float DropShadowDistance { 1.25f };

	StaticText::StaticText(ControlPtr pParent, fig::string_view text, FontFace fontFace, double ptSize, bool bAutoSize) : Control(pParent), TextBase(GetSDLTextEngine(), fontFace, ptSize),
		_bAutoSize(bAutoSize)
	{
		SetBackgroundColor(Color::Transparent);
		SetHeight(_lineSkip);
		SetText(text);
	}

	StaticText::~StaticText()
	{
		ReleaseTextures();
	}

	void StaticText::ReleaseTextures()
	{
		_textures.clear();
	}

	void StaticText::Reset()
	{
		ClearText();
		ReleaseTextures();
	};

	void StaticText::SetText(fig::string_view text)
	{
		if (not _bMultiline)
		{
			size_t newlinePos = text.find('\n', 0);
			text = fig::string_view { text.data(), std::min(text.length(), newlinePos) };
		}

		TextBase::SetText(text);
		InvalidateText();
		InvalidateLayout();
	}

	void StaticText::SetTextAndResize(fig::string_view text)
	{
		TextBase::SetText(text);
		fig::coord newWidth {};
		fig::coord newHeight {};
		DrawText(newWidth, newHeight);
		SetSize(newWidth, newHeight);
		_bInvalidated = false;
	}

	void StaticText::SetTextAndResize(fig::string_view text, fig::coord& newWidth, fig::coord& newHeight)
	{
		TextBase::SetText(text);
		DrawText(newWidth, newHeight);
		SetSize(newWidth, newHeight);
		_bInvalidated = false;
	}

	void StaticText::OnUpdate(float fElapsed)
	{
		if (_bInvalidated)
		{
			_bInvalidated = false;
			fig::coord tmpX, tmpY;
			DrawText(tmpX, tmpY);
		}
	}

	void StaticText::OnRender(fig::renderer_ptr pRenderer)
	{
		auto fgColor = GetForegroundColor();
		auto bgColor = GetBackgroundColor();
		if (bgColor && bgColor.a() != 0)
			DrawBackground(pRenderer);
		
		if (not _textures.empty())
		{
			auto alignRect = GetAlignedRect();
			for (size_t i = 0uz; i < _textures.size(); ++i)
			{
				if (_textures[i].empty())
					continue;

				auto lineRect = alignRect;
				lineRect.y += _lineSkip * toI(i);
				lineRect.w = _textures[i]->w;
				lineRect.h = _textures[i]->h;
				if ((_alignment & HorizontalAlignment::TextAlignRight) != 0)
					lineRect.x = alignRect.x + alignRect.w - lineRect.w;
				else if ((_alignment & HorizontalAlignment::TextAlignCenter) != 0)
					lineRect.x = alignRect.x + (alignRect.w - lineRect.w) / 2;

				auto lineRectf = to_rectf(lineRect);

				auto pTexture = _textures[i].get();

				if (_bDropShadow)
				{
					SDL_SetTextureColorMod(pTexture, 0, 0, 0);
					SDL_SetTextureAlphaMod(pTexture, 0xC0);

					auto shadowRect = lineRectf;
					shadowRect.x += DropShadowDistance;
					shadowRect.y += DropShadowDistance;
					SDL_RenderTexture(pRenderer, pTexture, NULL, &shadowRect);
				}
				SDL_SetTextureBlendMode(pTexture, SDL_BLENDMODE_BLEND);
				SDL_SetTextureColorMod(pTexture, 0xFF, 0xFF, 0xFF);
				SDL_SetTextureAlphaMod(pTexture, 0xFF);
				SDL_RenderTexture(pRenderer, pTexture, NULL, &lineRectf);
			}
		}
	}

	void StaticText::DrawText(fig::coord& newWidth, fig::coord& newHeight)
	{
		newWidth = 0;
		newHeight = 0;
		_textures.clear();
		if (_text.empty())
		{
			_textWidth = 0;
			_textHeight = 0;
			return;
		}

		auto fgColor = GetForegroundColor();
		auto bgColor = GetBackgroundColor();
		auto pRenderer = GetSDLRenderer();

		_textures.resize(_lines.size());

		newHeight = _lineSkip * toI(_lines.size());
		for (size_t index = 0uz; index != _lines.size(); ++index)
		{
			fig::coord w, h;
			DrawText(index, pRenderer, fgColor, bgColor, w, h);
			newWidth = std::max(newWidth, w);
		}

		_textWidth = newWidth;
		_textHeight = newHeight;
		newWidth += GetMarginHorizontal();
		newHeight += GetMarginVertical();
		if (_bAutoSize)
			SetSize(newWidth, newHeight);
	}

	void StaticText::DrawText(size_t line_index, fig::renderer_ptr pRenderer, const fig::color_ref_with_alpha& fgColor, const fig::color_ref_with_alpha& bgColor, fig::coord& newWidth, fig::coord& newHeight)
	{
		auto& line = _lines[line_index];
		auto& texture = _textures[line_index];

		const char* pText = _text.data();
		std::advance(pText, line.position);
		size_t textLength = line.length;

		std::string altText;
		if (_bEllipsis)
		{
			altText = GetEllipsisText(pText);
			pText = altText.c_str();
			textLength = altText.length();
		}

		if (line.length > 0 and pText[line.length - 1] == '\n')
			textLength -= 1;

		newWidth = 0;
		newHeight = 0;
		if (textLength == 0uz)
			return;

		if (fgColor)
		{
			if (bgColor.a() == 0xFF) // Opaque background: Use ClearType
			{
				if (SDL_Surface* pSurface = TTF_RenderText_LCD(_pFont, pText, textLength, fgColor, bgColor))
				{
					newWidth = pSurface->w;
					newHeight = pSurface->h;

					texture.reset(SDL_CreateTextureFromSurface(pRenderer, pSurface));
					SDL_DestroySurface(pSurface);
					return;
				}
			}
			else
			{
				if (SDL_Surface* pSurface = TTF_RenderText_Blended(_pFont, pText, textLength, fgColor))
				{
					newWidth = pSurface->w;
					newHeight = pSurface->h;
					texture.reset(SDL_CreateTextureFromSurface(pRenderer, pSurface));
					SDL_DestroySurface(pSurface);
					return;
				}
			}
		}

		newWidth = 0;
		newHeight = 0;
	}

	void StaticText::OnParent()
	{
		Control::OnParent();

		InvalidateText();
	}

	fig::rect StaticText::GetAlignedRect() const
	{
		auto& rect = GetRect();
		int x = toI(rect.x + GetMarginLeft());
		int y = toI(rect.y + GetMarginTop());
		int w = _textWidth;
		int h = _textHeight;
		fig::rect aligned_rect(x, y, w, h);

		if ((_alignment & HorizontalAlignment::TextAlignCenter) != 0)
			aligned_rect.x = x + (rect.w - w) / 2;
		else if ((_alignment & HorizontalAlignment::TextAlignRight) != 0)
			aligned_rect.x = x + rect.w - w;
		if ((_alignment & VerticalAlignment::TextAlignMiddle) != 0)
			aligned_rect.y = y + (rect.h - h) / 2;
		else if ((_alignment & VerticalAlignment::TextAlignBottom) != 0)
			aligned_rect.y = y + rect.h - h;
		return aligned_rect;
	}

	void StaticText::SetForegroundColor(fig::color_ref_with_alpha color)
	{
		Control::SetForegroundColor(color);
		InvalidateText();
	}

	void StaticText::SetBackgroundColor(fig::color_ref_with_alpha color)
	{
		Control::SetBackgroundColor(color);
		InvalidateText();
	}

	constexpr fig::string ellipsis(fig::string_view text, size_t utf8_length) noexcept
	{
		const char* pText = &text[0];
		for (size_t i = 1; i < utf8_length; ++i)
			SDL_StepUTF8(&pText, NULL);

		return fig::string { text.substr(0, ptrdiff_t(pText) - ptrdiff_t(&text[0])) } + "\u2026";
	}

	fig::string StaticText::GetEllipsisText(fig::string_view text) const
	{
		if (text.empty())
			return "";

		fig::coord maxWidth = std::max(_bAutoSize ? GetMaxWidth() : GetWidth(), 0);
		if (maxWidth == 0)
			return fig::string { text };

		int32_t w, h;
		if (TTF_GetStringSize(_pFont, text.data(), 0, &w, &h) and w <= maxWidth)
			return fig::string { text };

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
			testString = ellipsis(text, pos);
			if (TTF_GetStringSize(_pFont, testString.c_str(), 0, &w, &h) and w <= maxWidth)
				return testString;
		}
		return fig::string { text };
	}

	fig::point StaticText::MeasureText(bool bAllowEllipsis) const
	{
		if (_bEllipsis and bAllowEllipsis)
			return MeasureText(GetEllipsisText(_text));
		return MeasureText(_text);
	}

	fig::point StaticText::MeasureText(fig::string_view text) const
	{
		if constexpr (Disabled)
		{
			auto lines = LayoutParagraph(text);
			if (lines.empty())
				return fig::point(0, 0);

			fig::coord width = 0;
			fig::coord height = _fontHeight + _lineSkip * toI(lines.size() - 1uz);
			for (size_t index = 0uz; index < lines.size(); ++index)
			{
				auto& line = lines[index];
				const char* pText = _text.data();
				std::advance(pText, line.position);
				size_t textLength = line.length;
				if (line.length > 0 and pText[line.length - 1] == '\n')
					textLength -= 1;

				int32_t w, h;
				if (TTF_GetStringSize(_pFont, text.data(), textLength, &w, &h))
					width = std::max(width, w);
			}
			return fig::point { width, height };
		}
		else
		{
			if (_bWordWrap)
			{
				int32_t w, h;
				if (TTF_GetStringSizeWrapped(_pFont, text.data(), text.length(), GetMaxLineWidth(), &w, &h))
					return fig::point(w, h);
			}
			else
			{
				int32_t w, h;
				if (TTF_GetStringSize(_pFont, text.data(), text.length(), &w, &h))
					return fig::point(w, h);
			}
			return fig::point(0, 0);
		}
	}

	fig::coord StaticText::GetMaxLineWidth() const noexcept
	{
		return std::max(_wrapWidth > 0 ? _wrapWidth : (_bAutoSize ? GetMaxWidth() : GetWidth()), 0);
	}

	EventResult StaticText::OnEvent(fig::event& event)
	{
		if (IsUserEvent(event, UserEvent::ColorThemeChanged))
		{
			InvalidateText();
			return EventResult::Continue;
		}

		return EventResult::Pass;
	}

	void StaticText::EnableDropShadow(bool bEnable) noexcept 
	{ 
		_bDropShadow = bEnable; 
		InvalidateText(); 
	}

	void StaticText::EnableEllipsis(bool bEnable) noexcept 
	{ 
		_bEllipsis = bEnable; 
		InvalidateText(); 
	}
	
	void StaticText::EnableMultiline(bool bEnable) noexcept
	{
		_bMultiline = bEnable;
		InvalidateText();
	}

	void StaticText::EnableWordWrap(bool bEnable) noexcept 
	{ 
		_bWordWrap = bEnable;
		if (not bEnable)
			_wrapWidth = 0;
		InvalidateText(); 
	}

	void StaticText::OnSize()
	{
		if (_bWordWrap and not _bAutoSize)
			SetTextWrapWidth(std::max(GetClientRect().w, 0));
	}
}