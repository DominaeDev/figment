#pragma once

#include <functional>

#include "gui/Control.h"
#include "util/UndoStack.h"

namespace fig::gui
{
	class TextInput2 : public Control
	{
		struct TTFTextLine
		{
			fig::sdl::Text ttf_text;

			int32_t position; // in bytes
			int32_t length;
			bool eol {}; // End of paragraph
		};

		struct TTFCursor
		{
			int32_t position {}; // absolute
			int32_t offset {}; // relative to pText
			int32_t line {};
		};
	public:
		using TextChangedCallback = std::function<void(fig::string_view)>;
		using EnterPressedCallback = std::function<void(fig::string_view)>;

		enum class Flag
		{
			Single = 1 << 0,
			Multi = 1 << 1,
			Autosize = 1 << 2,
			Password = 1 << 3,
			CtrlEnterNewLine = 1 << 4,
		};
		using Flags = EnumFlags<Flag>;

		TextInput2(ControlPtr pParent, FontFace fontFace, double ptSize, Flags flags = {});
		~TextInput2();

		void SetText(fig::string_view text);
		void SetPlaceholder(fig::string_view text);
		void SetTextChangedCallback(TextChangedCallback cb);
		void SetEnterPressedCallback(EnterPressedCallback cb);
		void SetMinRows(int32_t rows);
		void SetMaxRows(int32_t rows);
		void SetTextWrapWidth(int32_t width);
		void SetFocus(bool focus);
		void SetFont(FontFace fontFace, double ptSize) noexcept;

		const fig::string& GetText() const noexcept { return _text; }
		int32_t GetTextWrapWidth() const noexcept;
		size_t GetLineCount() const noexcept;
		int32_t GetLineHeight() const noexcept { return _lineHeight; }

		void Select(int32_t start, int32_t end) noexcept;
		void SelectAll() noexcept;
		void Deselect() noexcept;
		std::pair<int32_t, int32_t> GetSelection() const noexcept { return std::make_pair(highlight_start, highlight_end); }
		bool HasSelection() const noexcept { return highlight_start >= 0 && highlight_end >= 0 && highlight_start != highlight_end; };

		bool Copy();
		bool Cut();
		bool Paste();
		void Clear();

		void Undo();
		void Redo();

	protected:
		void OnUpdate(float fElapsed) override;
		void OnRender(fig::renderer_ptr pRenderer) override;
		EventResult OnEvent(fig::event& event) override;
		void OnSize() override;
		void OnPostRender() override;
		void OnEnabled(bool bEnabled) override;

		virtual void OnText(fig::string_view text) {};

	private:
		void DrawText(fig::renderer_ptr pRenderer, TTF_Text* pText, int x, int y);
		void DrawPlaceholder(fig::renderer_ptr pRenderer, int x, int y);
		void DrawCursor(fig::renderer_ptr pRenderer);
		void DrawCandidates(fig::renderer_ptr pRenderer);
		void DrawComposition(fig::renderer_ptr pRenderer);
		void DrawCompositionCursor(fig::renderer_ptr pRenderer);
		void ClearCandidates();
		void SaveCandidates(const SDL_Event* event);
		void HandleComposition(const SDL_TextEditingEvent* event);
		void CancelComposition();
		void ResetComposition();
		void UpdateTextInputArea();
		void RefreshTexts() noexcept;

		int32_t SetCursor(int32_t index) noexcept;
		int32_t SetCursor(fig::point position) noexcept;

		void Insert(fig::string_view text);
		bool Delete();
		bool DeleteSelection();
		bool DeleteToNextWord();
		bool DeleteToEndOfLine();
		bool DeleteToEnd();
		bool Backspace();
		bool BackspaceToPriorWord();
		bool BackspaceToBeginning();
		bool BackspaceToBeginningOfLine();

		TTFCursor GetCursor() const noexcept;
		int32_t GetCursorPosition() const noexcept { return _cursor; }
		TTFCursor GetCursorAt(int32_t index) const noexcept;
		TTFCursor GetCursorAt(int32_t x, int32_t y) const noexcept;
		TTFCursor GetLineCursor(size_t line_index) const noexcept;

		int32_t MoveCursor(int32_t direction) noexcept;
		int32_t MoveCursorLeft() noexcept;
		int32_t MoveCursorRight() noexcept;
		int32_t MoveCursorUp() noexcept;
		int32_t MoveCursorDown() noexcept;
		int32_t MoveCursorBeginningOfLine() noexcept;
		int32_t MoveCursorEndOfLine() noexcept;
		int32_t MoveCursorToPriorWord() noexcept;
		int32_t MoveCursorToNextWord() noexcept;
		int32_t MoveCursorBeginning() noexcept;
		int32_t MoveCursorEnd() noexcept;
		void OnMoveCursor(int32_t last_position);

		fig::rectf GetCursorRect() const noexcept;
		void ResetCursorBlink();

		bool HandleMouseDown(int x, int y);
		bool HandleMouseMotion(int x, int y);
		bool HandleMouseUp(int x, int y);
		void ApplyScroll(int& x, int& y) const;
		void ApplyScroll(float& x, float& y) const;
		void ApplyScroll(fig::rect& rect) const;
		void ApplyScroll(fig::rectf& rect) const;
		void Autosize();
		void DidChange();

		bool IsMultiline() const noexcept { return _flags.IsSet(Flag::Multi) && not IsPassword(); }
		bool IsPassword() const noexcept { return _flags.IsSet(Flag::Password); }
		bool IsAutosized() const noexcept { return _flags.IsSet(Flag::Autosize); }
		bool IsWordWrapping() const noexcept { return IsMultiline() and _wrapWidth > 0; }

		// Layout
		void Insert(int32_t position, fig::string_view text);
		bool Delete(int32_t from, int32_t length);
		std::vector<TTFTextLine> LayoutParagraph(fig::string_view text);
		bool GetSelection(int32_t& marker, int32_t& length) const noexcept;
		void RelayoutAll();
		bool IsEOL(const TTFTextLine& line) const noexcept;

		fig::observer_ptr<TTF_Text> GetRenderedText();
		std::vector<fig::rectf> GetHighlights() const noexcept;

		// Password
		void UpdatePassword();
		int32_t ConvertToPasswordPosition(int32_t position) const;
		int32_t ConvertFromPasswordPosition(int32_t position) const;

	protected:
		fig::string _text;
		int32_t _lineHeight {};
		int32_t _wrapWidth {};
		std::vector<TTFTextLine> _lines;

		fig::observer_ptr<TTF_Font> _pFont;
		fig::observer_ptr<TTF_Text> _pPassword;
		fig::observer_ptr<TTF_Text> _pPlaceholder;
		size_t _lastLength = 0uz;

		bool _bFocused = false;
		bool _bIBeamCursor = false;
		Flags _flags {};
		bool _bAutoSize = false;
		fig::point _scroll {};
		int32_t _minRows = 1;
		int32_t _maxRows = 1;

		TextChangedCallback _pOnChanged = nullptr;
		EnterPressedCallback _pOnEnter = nullptr;

		// Cursor
		int32_t _cursor = 0;
		bool _cursor_visible = false;
		uint64_t _last_cursor_change = 0ULL;
		fig::rectf _cursor_rect {};

		// Selection
		int32_t highlight_start = -1;
		int32_t highlight_end = -1;
		bool _bIsHighlighting = false;

		// IME composition
		fig::sdl::Text _composition_text {};
		int32_t _composition_line {};
		int32_t _composition_start = -1;
		int32_t _composition_length = -1;
		int32_t _composition_cursor = -1;
		int32_t _composition_cursor_length = -1;
		fig::sdl::Text _candidates {};
		int32_t _selected_candidate_start = -1;
		int32_t _selected_candidate_length = -1;

		// Undo
		enum class UndoAction
		{
			Default, Write, WhitespacePunctuation, Erase,
		};

		struct UndoState
		{
			fig::string text;
			int cursor_pos;
			int highlight_start;
			int highlight_end;
			UndoAction actionType;
		};
		UndoStack<UndoState, UndoAction> _undo {};

		UndoState GetUndoState(UndoAction action) const noexcept;
		void InitUndo() noexcept;
		void PushUndo(UndoAction action, bool allowCoalesce = true);
	};
}
