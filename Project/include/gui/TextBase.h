#pragma once

#include <functional>

#include "util/UndoStack.h"

namespace fig::gui
{
	class TextBase
	{
	public:
		virtual void SetText(fig::string_view text);
		void SetTextWrapWidth(int32_t width);
		void SetFont(FontFace fontFace, double ptSize) noexcept;

		fig::string_view GetText() const noexcept { return _text; }
		int32_t GetTextWrapWidth() const noexcept;
		int32_t GetLineCount() const noexcept;
		int32_t GetLineHeight() const noexcept { return _lineHeight; }

		bool IsMultiline() const noexcept { return _bMultiline; }
		bool IsWordWrapping() const noexcept { return _bWordWrap and _wrapWidth > 0; }

	protected:
		struct TTFTextLine
		{
			fig::sdl::Text ttf_text;

			int32_t position; // in bytes
			int32_t length;
			bool eol {}; // End of paragraph
		};
	
		TextBase(fig::text_engine_ptr pTextEngine, FontFace fontFace = FontFace::Default, double ptSize = Constants::GUI::DefaultFontSize);
		virtual ~TextBase() {};

		void ClearText();
		void RefreshTexts() noexcept;

		std::vector<TTFTextLine> LayoutParagraph(fig::string_view text);
		void LayoutAll();
		bool IsEOL(const TTFTextLine& line) const noexcept;

		fig::text_engine_ptr _pTextEngine;
		fig::observer_ptr<TTF_Font> _pFont;
		fig::string _text;
		int32_t _lineHeight {};
		int32_t _wrapWidth {};

		bool _bMultiline = false;
		bool _bWordWrap = false;
		std::vector<TTFTextLine> _lines;
	};
}
