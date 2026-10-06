#pragma once

#include <functional>

#include "util/UndoStack.h"

namespace fig::gui
{
	struct TextStyle
	{
		fig::color_ref fgColor;
		// ...
	};

	using TextStyleId = size_t;

	struct StyleSpan
	{
		int32_t position;
		int32_t length;
		TextStyleId styleId;
	};

	class TextBase
	{
	public:
		virtual void SetText(fig::string_view text);
		virtual void SetStyledText(fig::string_view text, std::span<const StyleSpan> styleSpans);
		void SetFont(FontFace fontFace, double ptSize) noexcept;
		void SetTextWrapWidth(int32_t width);

		fig::string_view GetText() const noexcept { return _text; }
		fig::font_ptr GetFont() const { return _pFont.get(); }
		int32_t GetTextWrapWidth() const noexcept;

		bool IsWordWrapping() const noexcept { return _bWordWrap and _wrapWidth > 0; }
		void InvalidateText();

		void SetDefaultStyle(fig::color_ref fgColor) noexcept;
		TextStyleId AddStyle(fig::color_ref fgColor) noexcept;
		bool ApplyStyle(TextStyleId styleId, int32_t position, int32_t length) noexcept;

	protected:
		struct StyledTextRun
		{
			fig::sdl::Text ttf_text;
			int32_t position;
			int32_t length;
			TextStyleId styleId;
			int32_t byteOffset; // bytes, from line begin
			int32_t offsetX; // pixels
		};

		struct TTFTextLine
		{
			std::vector<StyledTextRun> runs;
			int32_t position; // in bytes
			int32_t length;
			bool eol {}; // End of paragraph
		};

		TextBase(fig::text_engine_ptr pTextEngine, FontFace fontFace = FontFace::Default, double ptSize = Constants::GUI::DefaultFontSize);
		virtual ~TextBase() {};

		void ClearText();
		void RefreshTexts() noexcept;

		virtual void OnRefreshedTexts() {};

		std::vector<TTFTextLine> LayoutParagraph(fig::string_view text) const;
		void LayoutAll();
		bool IsEOL(const TTFTextLine& line) const noexcept;

		void FinalizeLine(TTFTextLine& line) const;
		std::vector<StyledTextRun> StyleLine(const TTFTextLine& line) const;
		bool IsStyled() const noexcept { return not _styles.empty(); }
		const TextStyle& GetTextStyle(TextStyleId styleId) const;

		int32_t GetLineOffsetAt(const TTFTextLine& line, fig::coord px) const;
		int32_t GetPixelsToLineOffset(const TTFTextLine& line, int32_t offset) const;

		fig::text_engine_ptr _pTextEngine;
		fig::observer_ptr<TTF_Font> _pFont;
		fig::string _text;
		int32_t _fontHeight {};
		int32_t _lineSkip {};
		int32_t _wrapWidth {};
		bool _bInvalidated = true;

		bool _bWordWrap = false;
		std::vector<TTFTextLine> _lines;
		
		// Styling
		std::vector<StyleSpan> _styleSpans;
		std::vector<TextStyle> _styles;
	};
}
