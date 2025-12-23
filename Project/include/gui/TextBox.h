#ifndef TEXT_BOX_H__
#define TEXT_BOX_H__
#pragma once

#include <functional>

#include "gui/ControlWithMargins.h"
#include "util/UndoStack.h"
#include "util/StringUtility.h"

namespace fig::gui
{
	using EnterPressedCallback = std::function<void(fig::string)>;

	class TextBox : public ControlWithMargins
	{
	public:
		enum class Flag
		{ 
			Single	= 1 << 0,
			Multi	= 1 << 1,
		};
		using Flags = EnumFlags<Flag>;

		TextBox(Control* pParent, FontFace fontFace, double ptSize, Flags flags = { Flag::Single });
		~TextBox();

		void SetText(fig::string text);
		void SetEnterPressedCallback(EnterPressedCallback cb);

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
		void OnUpdate(float fDeltaTime) override;
		void OnRender(RendererPtr pRenderer) override;
		bool OnEvent(Event& event) override;
		void OnSize() override;

	private:
		void Insert(const char* text);

		void DrawText(RendererPtr pRenderer, TTF_Text* pText, float x, float y);
		void DrawCursor(RendererPtr pRenderer);
		void DrawCandidates(RendererPtr pRenderer);
		void DrawComposition(RendererPtr pRenderer);
		void DrawCompositionCursor(RendererPtr pRenderer);
		void ClearCandidates();
		void SaveCandidates(const SDL_Event* event);
		bool GetHighlightExtents(int* marker, int* length);
		void HandleComposition(const SDL_TextEditingEvent* event);
		void CancelComposition();
		void ResetComposition();
		void OnMoveCursor(int last);
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

		bool HandleMouseDown(float x, float y);
		bool HandleMouseMotion(float x, float y);
		bool HandleMouseUp(float x, float y);

		inline bool HasSelection() const { return highlight_start >= 0 && highlight_end >= 0 && highlight_start != highlight_end; };

		void ScrollPoint(int& x, int& y) const;
		void ScrollPoint(float& x, float& y) const;
		void ScrollPoint(Rectf& rect) const;
		inline bool IsMultiline() const { return _flags.IsSet(Flag::Multi); }

	protected:
		TTF_Font* _pFont {};
		TTF_Text* _pText {};
		bool _bFocused = false;
		bool _bIBeamCursor = false;
		Flags _flags {};
		bool _bAutoSize = false;
		Point _scroll {};

		Texture* _pTexture = nullptr;
		Surface* _pSurface = nullptr;
		EnterPressedCallback _pOnEnter = nullptr;

		/* Cursor support */
		int _cursor = 0;
		bool _cursor_visible = false;
		Uint64 _last_cursor_change {};
		Rectf _cursor_rect {};

		/* Highlight support */
		bool _bIsHighlighting = false; // Mouse-selection
		int highlight_start = -1;
		int highlight_end = -1;

		/* IME composition */
		int composition_start = -1;
		int composition_length = -1;
		int composition_cursor = -1;
		int composition_cursor_length = -1;

		/* IME candidates */
		TTF_Text* candidates {};
		int selected_candidate_start = -1;
		int selected_candidate_length = -1;

		enum class UndoAction
		{
			Default,
			Write,
			WhitespacePunctuation,
			Erase,
			Other,
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
#endif