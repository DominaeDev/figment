#pragma once

#include <functional>

#include "util/UndoStack.h"
#include "ITextStyleProvider.h"

namespace fig::gui
{
	class TextBase
	{
	public:
		virtual void SetText(fig::string_view text);
		virtual void SetStyledText(fig::string_view text, std::span<const StyleSpan> styleSpans);
		void SetFont(FontFace fontFace, double ptSize) noexcept;
		void SetTextWrapWidth(int32_t width);

		[[nodiscard]] fig::string_view GetText() const noexcept { return _text; }
		fig::font_ptr GetFont() const { return _pFont.get(); }
		int32_t GetTextWrapWidth() const noexcept;

		bool IsWordWrapping() const noexcept { return _bWordWrap and _wrapWidth > 0; }
		void InvalidateText();

		template <typename T>
		requires std::derived_from<T, ITextStyleProvider>
		void SetStyleProvider()
		{
			_pStyleProvider = std::make_unique<T>(this);
		}

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

		bool HasStyleProvider() const noexcept { return (bool)_pStyleProvider; }
		void ApplyStyle(fig::string_view text, size_t position = 0uz);
		std::vector<StyledTextRun> StyleLine(const TTFTextLine& line) const;
		void FinalizeLine(TTFTextLine& line) const;

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
		std::unique_ptr<ITextStyleProvider> _pStyleProvider;
	};
}
