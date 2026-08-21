#pragma once

#include "Figment.h"
#include "Fonts.h"
#include "gui/Control.h"

namespace fig::gui
{
	enum HorizontalAlignment : uint8_t
	{
		TextAlignLeft = (1u << 0),
		TextAlignCenter = (1u << 1),
		TextAlignRight = (1u << 2),
	};

	enum VerticalAlignment : uint8_t
	{
		TextAlignTop = (1u << 3),
		TextAlignMiddle = (1u << 4),
		TextAlignBottom = (1u << 5),
	};

	enum TextAlignment : unsigned short
	{
		LeftTop			= HorizontalAlignment::TextAlignLeft | VerticalAlignment::TextAlignTop,
		LeftCenter		= HorizontalAlignment::TextAlignLeft | VerticalAlignment::TextAlignMiddle,
		LeftBottom		= HorizontalAlignment::TextAlignLeft | VerticalAlignment::TextAlignBottom,
		MiddleTop		= HorizontalAlignment::TextAlignCenter | VerticalAlignment::TextAlignTop,
		MiddleCenter	= HorizontalAlignment::TextAlignCenter | VerticalAlignment::TextAlignMiddle,
		MiddleBottom	= HorizontalAlignment::TextAlignCenter | VerticalAlignment::TextAlignBottom,
		RightTop		= HorizontalAlignment::TextAlignRight | VerticalAlignment::TextAlignTop,
		RightCenter		= HorizontalAlignment::TextAlignRight | VerticalAlignment::TextAlignMiddle,
		RightBottom		= HorizontalAlignment::TextAlignRight | VerticalAlignment::TextAlignBottom,
		Default			= LeftTop,
	};

	class StaticText : public Control
	{
	public:
		StaticText(ControlPtr pParent, fig::string_view text, FontFace fontFace = FontFace::Default, double ptSize = Constants::GUI::DefaultFontSize, bool bAutoSize = true);
		virtual ~StaticText();

		fig::font_ptr GetFont() const { return _pFont.get(); }

		void SetText(fig::string_view text);
		void SetTextAndResize(fig::string_view text);
		void SetTextAndResize(fig::string_view text, fig::coord& newWidth, fig::coord& newHeight);

		const fig::string& GetText() const { return _text; }

		void SetAlignment(TextAlignment alignment) { _alignment = alignment; }
		void SetFont(fig::font_ptr pFont) { _pFont.reset(pFont); }

		void SetForegroundColor(fig::color color) override;
		void SetBackgroundColor(fig::color color) override;
		void EnableDropShadow(bool bEnable) noexcept { _bDropShadow = bEnable; _bInvalidated = true; }
		void EnableEllipsis(bool bEnable) noexcept { _bEllipsis = bEnable; _bInvalidated = true; }
		void EnableWordWrap(bool bEnable) noexcept { _bWordWrap = bEnable; _bInvalidated = true; }
		void SetMaxLineWidth(fig::coord width) { _maxLineWidth = width; _bWordWrap |= width > 0; }

		bool IsEllipsisEnabled() const noexcept { return _bEllipsis; }
		bool IsWordWrapEnabled() const noexcept { return _bWordWrap; }

		fig::point MeasureText(bool bAllowEllipsis = true) const;
		void Reset();
		void InvalidateText();

	protected:
		fig::rectf GetAlignedRect() const;

		void OnUpdate(float fElapsed) override;
		void OnRender(fig::renderer_ptr pRenderer) override;
		void OnParent() override;
		fig::coord GetMaxLineWidth() const noexcept;
	private:
		void DrawText(fig::coord& textWidth, fig::coord& textHeight);
		void DrawShadow(const char* pText);
		fig::string GetEllipsisText(const fig::string& text) const;
		void ReleaseTexture();

		fig::string _text;
		bool _bInvalidated = false;
		bool _bAutoSize = true;
		bool _bDropShadow = false;
		bool _bEllipsis = false;
		bool _bWordWrap = false;
		fig::coord _maxLineWidth = 0;

		fig::observer_ptr<TTF_Font> _pFont;
		fig::sdl::Texture _texture {};
		fig::sdl::Texture _shadow {};
		int _textWidth;
		int _textHeight;

		TextAlignment _alignment = TextAlignment::Default;
	};
}