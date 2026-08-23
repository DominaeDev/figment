#pragma once

#include <functional>

#include "gui/Control.h"
#include "util/UndoStack.h"

namespace fig::gui
{
	using TextChangedCallback = std::function<void(fig::string)>;
	using EnterPressedCallback = std::function<void(fig::string)>;

	class TextInput : public Control
	{
	public:
		enum class Flag
		{ 
			Single				= 1 << 0,
			Multi				= 1 << 1,
			Autosize			= 1 << 2,
			Password			= 1 << 3,
			CtrlEnterNewLine	= 1 << 4,
		};
		using Flags = EnumFlags<Flag>;

		TextInput(ControlPtr pParent, FontFace fontFace, double ptSize, Flags flags = {});
		~TextInput();

		void SetText(fig::string_view text);
		void SetPlaceholder(fig::string_view text);
		void SetTextChangedCallback(TextChangedCallback cb);
		void SetEnterPressedCallback(EnterPressedCallback cb);
		void SetMinRows(int32_t rows);
		void SetMaxRows(int32_t rows);

		fig::string GetText() const;

		void Clear();
		void SetFocus(bool focus);
		void MoveCursorBeginningOfLine();
		void MoveCursorEndOfLine();
		void MoveCursorBeginning();
		void MoveCursorEnd();
		void SelectAll();
		void Deselect();
		bool DeleteHighlight();
		void Copy();
		void Cut();
		void Paste();
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
		void Insert(fig::string_view text);
		void DrawText(fig::renderer_ptr pRenderer, TTF_Text* pText, int x, int y);
		void DrawPlaceholder(fig::renderer_ptr pRenderer, int x, int y);
		void DrawCursor(fig::renderer_ptr pRenderer);
		void DrawCandidates(fig::renderer_ptr pRenderer);
		void DrawComposition(fig::renderer_ptr pRenderer);
		void DrawCompositionCursor(fig::renderer_ptr pRenderer);
		void ClearCandidates();
		void SaveCandidates(const SDL_Event* event);
		bool GetHighlightExtents(int* marker, int* length);
		void HandleComposition(const SDL_TextEditingEvent* event);
		void CancelComposition();
		void ResetComposition();
		void OnMoveCursor(int last_cursor);
		void UpdateTextInputArea();
		void SetCursorPosition(int position);
		void MoveCursorIndex(int direction);
		void MoveCursorLeft();
		void MoveCursorRight();
		void MoveCursorUp();
		void MoveCursorDown();
		void MoveCursorToPriorWord();
		void MoveCursorToNextWord();
		void Backspace();
		void BackspaceToBeginning();
		void BackspaceToBeginningOfLine();
		void BackspaceToPriorWord();
		void DeleteToEnd();
		void DeleteToEndOfLine();
		void DeleteToNextWord();
		void Delete();
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

		inline bool HasSelection() const noexcept { return highlight_start >= 0 && highlight_end >= 0 && highlight_start != highlight_end; };
		inline bool IsMultiline() const noexcept { return _flags.IsSet(Flag::Multi) && not IsPassword(); }
		inline bool IsPassword() const noexcept { return _flags.IsSet(Flag::Password); }
		inline bool IsAutosized() const noexcept { return _flags.IsSet(Flag::Autosize); }

		TTF_Text* GetRenderedText();
		void UpdatePassword();
		int32_t ConvertToPasswordPosition(int32_t position);
		int32_t ConvertFromPasswordPosition(int32_t position);

	protected:
		fig::observer_ptr<TTF_Font> _pFont;
		fig::observer_ptr<TTF_Text> _pText;
		fig::observer_ptr<TTF_Text> _pPassword;
		fig::observer_ptr<TTF_Text> _pPlaceholder;
		size_t _lastLength = 0uz;
		
		bool _bFocused = false;
		bool _bIBeamCursor = false;
		Flags _flags {};
		bool _bAutoSize = false;
		fig::point _scroll {};
		int _minRows = 1;
		int _maxRows = 1;

		fig::texture_ptr _pTexture;
		fig::surface_ptr _pSurface;
		TextChangedCallback _pOnChanged = nullptr;
		EnterPressedCallback _pOnEnter = nullptr;

		// Cursor
		int _cursor = 0;
		bool _cursor_visible = false;
		uint64_t _last_cursor_change = 0ULL;
		fig::rectf _cursor_rect {};

		// Selection
		bool _bIsHighlighting = false;
		int highlight_start = -1;
		int highlight_end = -1;

		// IME composition
		int composition_start = -1;
		int composition_length = -1;
		int composition_cursor = -1;
		int composition_cursor_length = -1;

		TTF_Text* candidates {};
		int selected_candidate_start = -1;
		int selected_candidate_length = -1;

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
