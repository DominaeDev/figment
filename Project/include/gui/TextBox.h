#ifndef TEXT_BOX_H__
#define TEXT_BOX_H__
#pragma once

#include <functional>

#include "ControlWithMargins.h"

namespace fig::gui
{
	using EnterPressedCallback = std::function<void(fig::string)>;

	class TextBox : public ControlWithMargins
	{
	public:
		TextBox(Control* pParent, FontFace fontFace, double ptSize);
		~TextBox();

		void SetEnterPressedCallback(EnterPressedCallback cb);

		void SetText(fig::string text);

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

	protected:
		void OnUpdate(float fDeltaTime) override;
		void OnRender(Renderer* pRenderer) override;
		bool OnEvent(Event& event) override;
		void OnSize() override;

	private:
		void Insert(const char* text);

		void DrawText(Renderer* pRenderer, TTF_Text* pText, float x, float y);
		void DrawCursor(Renderer* pRenderer);
		void DrawCandidates(Renderer* pRenderer);
		void DrawComposition(Renderer* pRenderer);
		void DrawCompositionCursor(Renderer* pRenderer);
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


		bool HandleMouseDown(float x, float y);
		bool HandleMouseMotion(float x, float y);
		bool HandleMouseUp(float x, float y);

	protected:
		TTF_Font* _pFont {};
		TTF_Text* _pText {};
		bool _bFocused = false;
		bool _bIBeamCursor = false;

		Texture* _pTexture = nullptr;
		Surface* _pSurface = nullptr;
		EnterPressedCallback _pOnEnter = nullptr;

		/* Cursor support */
		int _cursor = 0;
		bool _cursor_visible = false;
		Uint64 _last_cursor_change {};
		Rectf _cursor_rect {};

		/* Highlight support */
		bool _bIsHighlighting = false;
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
	};
}
#endif