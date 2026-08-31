#pragma once

#include "gui/GUITypes.h"

// Helper class for dealing with TTF_Text's severe performance issues with long texts.
namespace fig::gui
{
	struct TTFTextLine
	{
		fig::sdl::Text ttf_text;

		int32_t position; // in bytes
		int32_t length;
		bool eol {}; // End of paragraph
		bool dirty {};
	};

	struct TTFCursor
	{
		TTF_Text* pText { nullptr };
		int32_t position { -1 }; // absolute
		int32_t offset { -1 }; // relative to pText
		int32_t line { 0 };
	};

	class TTFTextBody
	{
	public:
		TTFTextBody() = default;
		TTFTextBody(fig::text_engine_ptr pTextEngine, fig::observer_ptr<TTF_Font> pFont);

		void SetText(fig::string_view text);
		void SetTextWrapWidth(int32_t width);
		int32_t GetTextWrapWidth() const noexcept;
		const fig::string& GetText() const noexcept { return _text; }

		fig::observer_ptr<TTF_Text> GetRenderedText() noexcept; //! @temp

		const std::vector<TTFTextLine>& GetLines() const noexcept;
		size_t GetLineCount() const noexcept;
		int32_t GetLineHeight() const noexcept { return _lineHeight; }

		bool IsWordWrapping() const noexcept { return _wrapWidth > 0; }

		void Render(fig::renderer_ptr pRenderer);

		int32_t SetCursor(int32_t index) noexcept;
		bool SetCursor(fig::point position) noexcept;

		TTFCursor GetCursor() const noexcept;
		int32_t GetCursorPosition() const noexcept { return _cursor; }
		TTFCursor GetCursorAt(int32_t index) const noexcept;
		TTFCursor GetCursorAt(int32_t x, int32_t y) const noexcept;
		TTFCursor GetLineCursor(size_t line_index) const noexcept;

		int32_t MoveCursor(int direction) noexcept;
		int32_t MoveCursorLeft() noexcept;
		int32_t MoveCursorRight() noexcept;
		int32_t MoveCursorUp() noexcept;
		int32_t MoveCursorDown() noexcept;
		int32_t MoveCursorBeginningOfLine() noexcept;
		int32_t MoveCursorEndOfLine() noexcept;
		int32_t MoveCursorToPriorWord() noexcept;
		int32_t MoveCursorToNextWord() noexcept;

		void Select(int32_t start, int32_t end) noexcept;
		void SelectAll() noexcept;
		void Deselect() noexcept;
		bool HasSelection() const noexcept { return highlight_start >= 0 && highlight_end >= 0 && highlight_start != highlight_end; };
		std::pair<int32_t, int32_t> GetSelection() const noexcept { return std::make_pair(highlight_start, highlight_end); }
		std::vector<fig::rectf> GetHighlights() const noexcept;
		fig::rectf GetCursorRect() const noexcept;

		bool Delete();
		bool DeleteSelection();
		bool DeleteToNextWord();
		bool DeleteToEndOfLine();
		bool DeleteToEnd();
		bool Backspace();
		bool BackspaceToPriorWord();
		bool BackspaceToBeginning();
		bool BackspaceToBeginningOfLine();
		void Insert(fig::string_view text);

		void Copy();
		bool Cut();
		void Clear();

	private:
		void Insert(int32_t position, fig::string_view text);
		bool Delete(int32_t from, int32_t length);
		std::vector<TTFTextLine> LayoutParagraph(fig::string_view text);
		bool GetSelection(int32_t& marker, int32_t& length) const noexcept;
		void Relayout();
		bool IsEOL(const TTFTextLine& line) const noexcept;

		void InvalidateAt(int32_t position, int32_t length) noexcept;
	
		fig::text_engine_ptr _pTextEngine;
		fig::observer_ptr<TTF_Font> _pFont;
		fig::string _text;
		int32_t _lineHeight {};
		int32_t _wrapWidth {};
		std::vector<TTFTextLine> _lines;
		bool _bInvalidated { false };

		int32_t _cursor = 0;
		int32_t highlight_start = -1;
		int32_t highlight_end = -1;
	};


};