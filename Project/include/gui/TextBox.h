#pragma once
#include <functional>

#include "ControlWithMargins.h"
#include "Fonts.h"
#include "Types.h"

struct SDL_Renderer;

typedef std::function<void(fig::string)> EnterPressedCallback;

class TextBox : public ControlWithMargins
{
public:
	TextBox(Control* pParent, FontFace fontFace, double ptSize);
	virtual ~TextBox();

	void SetEnterPressedCallback(EnterPressedCallback cb);

	void SetText(fig::string text);

	void Clear();
	void SetFocus(bool focus);
	void MoveCursorBeginningOfLine();
	void MoveCursorEndOfLine();
	void MoveCursorBeginning();
	void MoveCursorEnd();
	void Backspace();
	void BackspaceToBeginning();
	void DeleteToEnd();
	void Delete();
	void SelectAll();
	bool DeleteHighlight();
	void Copy();
	void Cut();
	void Paste();

protected:
	void OnUpdate(float fDeltaTime) override;
	void OnRender(Renderer* pRenderer) override;
	bool OnEvent(SDL_Event* event) override;
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

	bool HandleMouseDown(float x, float y);
	bool HandleMouseMotion(float x, float y);
	bool HandleMouseUp(float x, float y);

protected:
	TTF_Font* _pFont;
	TTF_Text* _pText;
	bool _bFocused = false;
	bool _bIBeamCursor = false;

	Texture* _pTexture = nullptr;
	Surface* _pSurface = nullptr;
	EnterPressedCallback _pOnEnter = nullptr;

	/* Cursor support */
	int _cursor = 0;
	bool _cursor_visible;
	Uint64 _last_cursor_change;
	Rectf _cursor_rect;

	/* Highlight support */
	bool _bIsHighlighting = false;
	int highlight_start;
	int highlight_end;

	/* IME composition */
	int composition_start;
	int composition_length;
	int composition_cursor;
	int composition_cursor_length;

	/* IME candidates */
	TTF_Text* candidates;
	int selected_candidate_start;
	int selected_candidate_length;
};