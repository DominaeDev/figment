#pragma once

#include "Figment.h"
#include "Fonts.h"
#include "gui/Control.h"
#include "gui/Textbase.h"

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

	class StaticText : public Control, public TextBase
	{
	public:
		StaticText(ControlPtr pParent, fig::string_view text, FontFace fontFace = FontFace::Default, double ptSize = Constants::GUI::DefaultFontSize, bool bAutoSize = true);
		virtual ~StaticText();

		void SetText(fig::string_view text) override;
		void SetTextAndResize(fig::string_view text);
		void SetTextAndResize(fig::string_view text, fig::coord& newWidth, fig::coord& newHeight);

		void SetAlignment(TextAlignment alignment) { _alignment = alignment; }

		void SetForegroundColor(fig::color_ref_with_alpha color) override;
		void SetBackgroundColor(fig::color_ref_with_alpha color) override;
		void EnableDropShadow(bool bEnable) noexcept;

		void EnableEllipsis(bool bEnable) noexcept;
		bool IsEllipsisEnabled() const noexcept { return _bEllipsis; }
		void EnableMultiline(bool bEnable) noexcept;
		void EnableWordWrap(bool bEnable) noexcept;

		fig::point MeasureText(bool bAllowEllipsis = true) const;
		fig::point MeasureText(fig::string_view text) const;
		void Reset();

	protected:
		void OnUpdate(float fElapsed) override;
		void OnRender(fig::renderer_ptr pRenderer) override;
		void OnParent() override;
		void OnSize() override;
		EventResult OnEvent(fig::event& event) override;

		fig::rectf GetAlignedRect() const;
	private:
		void DrawText(fig::coord& textWidth, fig::coord& textHeight);
		void DrawText(size_t line_index, fig::renderer_ptr pRenderer, const fig::color_ref_with_alpha& fgColor, const fig::color_ref_with_alpha& bgColor, fig::coord& textWidth, fig::coord& textHeight);
		void DrawShadow(size_t line_index, fig::renderer_ptr pRenderer);
		fig::string GetEllipsisText(fig::string_view text) const;
		void ReleaseTextures();
		fig::coord GetMaxLineWidth() const noexcept;

		bool _bMultiline = true;
		bool _bAutoSize = true;
		bool _bDropShadow = false;
		bool _bEllipsis = false;
		
		std::vector<fig::sdl::Texture> _textures {};
		std::vector<fig::sdl::Texture> _shadows {};
		int _textWidth;
		int _textHeight;

		TextAlignment _alignment = TextAlignment::Default;
	};
}