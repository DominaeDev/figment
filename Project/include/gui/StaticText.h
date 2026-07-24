#pragma once

#include "Figment.h"
#include "Fonts.h"
#include "Control.h"

namespace fig::gui
{
	enum HorizontalAlignment : unsigned short
	{
		Left = (1u << 0),
		Center = (1u << 1),
		Right = (1u << 2),
	};

	enum VerticalAlignment : unsigned short
	{
		Top = (1u << 3),
		Middle = (1u << 4),
		Bottom = (1u << 5),
	};

	enum TextAlignment : unsigned short
	{
		Left_Top = HorizontalAlignment::Left | VerticalAlignment::Top,
		Left_Center = HorizontalAlignment::Left | VerticalAlignment::Middle,
		Left_Bottom = HorizontalAlignment::Left | VerticalAlignment::Bottom,
		Middle_Top = HorizontalAlignment::Center | VerticalAlignment::Top,
		Middle_Center = HorizontalAlignment::Center | VerticalAlignment::Middle,
		Middle_Bottom = HorizontalAlignment::Center | VerticalAlignment::Bottom,
		Right_Top = HorizontalAlignment::Right | VerticalAlignment::Top,
		Right_Center = HorizontalAlignment::Right | VerticalAlignment::Middle,
		Right_Bottom = HorizontalAlignment::Right | VerticalAlignment::Bottom,
		Default = Left_Top,
	};

	class StaticText : public Control
	{
	public:
		StaticText(ControlPtr pParent, const fig::string& text, FontFace fontFace, double ptSize, bool bAutoSize = true);
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
		void EnableWordWrap(bool bEnable) noexcept { _bWordWrap = bEnable; _bInvalidated = true; }
		void EnableEllipsis(bool bEnable) noexcept { _bEllipsis = bEnable; _bInvalidated = true; }

		fig::point MeasureText(bool bAllowEllipsis = true) const;
		void Reset();
		void InvalidateText();

	protected:
		fig::rectf GetAlignedRect() const;

		void OnUpdate(float fElapsed) override;
		void OnRender(fig::renderer_ptr pRenderer) override;
		void OnParent() override;

	private:
		void DrawText(fig::coord& textWidth, fig::coord& textHeight);
		void DrawShadow(const char* pText);
		fig::string GetEllipsisText(const fig::string& text) const;
		void ReleaseTexture();

		fig::string _text;
		bool _bInvalidated = false;
		bool _bAutoSize = true;
		bool _bDropShadow = false;
		bool _bWordWrap = false;
		bool _bEllipsis = false;

		fig::observer_ptr<TTF_Font> _pFont;
		fig::sdl::Texture _texture {};
		fig::sdl::Texture _shadow {};
		int _textWidth;
		int _textHeight;

		TextAlignment _alignment = TextAlignment::Default;
	};
}