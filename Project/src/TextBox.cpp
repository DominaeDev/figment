#include "Textbox.h"
#include "Text.h"
#include "AppState.h"
#include "Color.h"
#include <algorithm>

#define CURSOR_BLINK_INTERVAL_MS    500

TextBox::TextBox(FontFace fontFace, double ptSize)
{
	_pFont = Fonts::GetFont(fontFace, ptSize);
	_pText = TTF_CreateText(Text::GetEngine(), _pFont, "Hahaha hello this is me bitch!", 0);

	_bFocused = false;

	highlight_start = -1;
	highlight_end = -1;

	/* Wrap the editbox text within the editbox area */
	TTF_SetTextWrapWidth(_pText, (int)SDL_floorf(_rect.w));

	/* Show the whitespace when wrapping, so it can be edited */
	TTF_SetTextWrapWhitespaceVisible(_pText, true);
}

TextBox::~TextBox()
{
	ClearCandidates();
	TTF_DestroyText(_pText);
}

void TextBox::OnUpdate(float fDeltaTime)
{
	Uint64 now = SDL_GetTicks();
	if ((now - _last_cursor_change) >= CURSOR_BLINK_INTERVAL_MS)
	{
		_cursor_visible = !_cursor_visible;
		_last_cursor_change = now;
	}

	if (_bFocused)
	{
		/* Calculate the cursor rect, used for positioning candidates */
		TTF_SubString cursor;
		if (TTF_GetTextSubString(_pText, _cursor, &cursor))
		{
			SDL_FRect cursor_rect;
			SDL_RectToFRect(&cursor.rect, &cursor_rect);
			cursor_rect.x += _rect.x;
			cursor_rect.y += _rect.y;
			cursor_rect.w = 1.0f;
			auto hehe = TTF_GetFontProperties(_pFont);
			cursor_rect.h = std::max(cursor_rect.h, (float)TTF_GetFontLineSkip(_pFont));
			SDL_copyp(&_cursor_rect, &cursor_rect);

			UpdateTextInputArea();
		}
	}
}

void TextBox::OnRender(SDL_Renderer* pRenderer)
{
	ClearBackground(pRenderer);
	
	// Draw highlight(s) 
	int marker, length;
	if (GetHighlightExtents(&marker, &length))
	{
		TTF_SubString** pHighlights = TTF_GetTextSubStringsForRange(_pText, marker, length, NULL);
		if (pHighlights)
		{
			int i;
			SDL_SetRenderDrawColor(pRenderer, Color::TextSelectionBackground.r, Color::TextSelectionBackground.g, Color::TextSelectionBackground.b, Color::TextSelectionBackground.a);
			for (i = 0; pHighlights[i]; ++i)
			{
				SDL_FRect rect;
				SDL_RectToFRect(&pHighlights[i]->rect, &rect);
				rect.x += _rect.x;
				rect.y += _rect.y;
				SDL_RenderFillRect(pRenderer, &rect);
			}
			SDL_free(pHighlights);
		}
	}

	DrawText(pRenderer, _pText, _rect.x, _rect.y);

	if (_bFocused)
	{
		if (composition_length > 0)
			DrawComposition(pRenderer);

		if (candidates)
			DrawCandidates(pRenderer);

		if (_cursor_visible)
			DrawCursor(pRenderer);
	}
}

static bool IsShiftDown()
{
	return (SDL_GetModState() & SDL_KMOD_SHIFT) != 0;
}

void TextBox::DrawText(SDL_Renderer* pRenderer, TTF_Text* pText,  float x, float y)
{
	auto fgColor = GetForegroundColor();

	TTF_SetTextColor(pText, fgColor.r, fgColor.g, fgColor.b, fgColor.a);
	TTF_DrawRendererText(pText, _rect.x, _rect.y);
}

bool TextBox::GetHighlightExtents(int* marker, int* length)
{
	if (highlight_start >= 0 && highlight_end >= 0)
	{
		int marker1 = SDL_min(highlight_start, highlight_end);
		int marker2 = SDL_max(highlight_start, highlight_end);
		if (marker2 > marker1)
		{
			*marker = marker1;
			*length = marker2 - marker1;
			return true;
		}
	}
	return false;
}

void TextBox::ResetComposition()
{
	composition_start = 0;
	composition_length = 0;
	composition_cursor = 0;
	composition_cursor_length = 0;
}

static int UTF8ByteLength(const char* text, int num_codepoints)
{
	const char* start = text;
	while (num_codepoints > 0)
	{
		Uint32 ch = SDL_StepUTF8(&text, NULL);
		if (ch == 0)
		{
			break;
		}
		--num_codepoints;
	}
	return (int)(uintptr_t)(text - start);
}

void TextBox::HandleComposition(const SDL_TextEditingEvent* event)
{
	DeleteHighlight();

	if (composition_length > 0)
	{
		TTF_DeleteTextString(_pText, composition_start, composition_length);
		ResetComposition();
	}

	int length = (int)SDL_strlen(event->text);
	if (length > 0)
	{
		composition_start = _cursor;
		composition_length = length;
		TTF_InsertTextString(_pText, composition_start, event->text, composition_length);
		if (event->start > 0 || event->length > 0)
		{
			composition_cursor = UTF8ByteLength(&_pText->text[composition_start], event->start);
			composition_cursor_length = UTF8ByteLength(&_pText->text[composition_start + composition_cursor], event->length);
		}
		else
		{
			composition_cursor = length;
			composition_cursor_length = 0;
		}
	}
}

void TextBox::CancelComposition()
{
	ResetComposition();

	SDL_ClearComposition(Application::GetWindow());
}

void TextBox::DrawComposition(SDL_Renderer* pRenderer)
{
	/* Draw an underline under the composed text */
	int font_height = TTF_GetFontHeight(_pFont);
	TTF_SubString** substrings = TTF_GetTextSubStringsForRange(_pText, composition_start, composition_length, NULL);
	if (substrings)
	{
		for (int i = 0; substrings[i]; ++i)
		{
			SDL_FRect rect;
			SDL_RectToFRect(&substrings[i]->rect, &rect);
			rect.x += rect.x;
			rect.y += (rect.y + font_height);
			rect.h = 1.0f;
			SDL_RenderFillRect(pRenderer, &rect);
		}
		SDL_free(substrings);
	}

	/* Thicken the underline under the active clause in the composed text */
	if (composition_cursor_length > 0)
	{
		substrings = TTF_GetTextSubStringsForRange(_pText, composition_start + composition_cursor, composition_cursor_length, NULL);
		if (substrings)
		{
			for (int i = 0; substrings[i]; ++i)
			{
				SDL_FRect rect;
				SDL_RectToFRect(&substrings[i]->rect, &rect);
				rect.x += rect.x;
				rect.y += (rect.y + font_height) - 1;
				rect.h = 8.0f;
				SDL_RenderFillRect(pRenderer, &rect);
			}
			SDL_free(substrings);
		}
	}
}

void TextBox::DrawCompositionCursor(SDL_Renderer* pRenderer)
{
	if (composition_cursor_length == 0)
	{
		TTF_SubString cursor;
		if (TTF_GetTextSubString(_pText, composition_start + composition_cursor, &cursor))
		{
			SDL_FRect rect;
			SDL_RectToFRect(&cursor.rect, &rect);
			rect.x += rect.x;
			rect.y += rect.y;
			rect.w = 1.0f;

			SDL_SetRenderDrawColor(pRenderer, 0, 0, 0, 0xFF);
			SDL_RenderFillRect(pRenderer, &rect);
		}
	}
}

void TextBox::ClearCandidates()
{
	if (candidates)
	{
		TTF_DestroyText(candidates);
		candidates = NULL;
	}
	selected_candidate_start = 0;
	selected_candidate_length = 0;
}

void TextBox::SaveCandidates(const SDL_Event* event)
{
	int i;

	ClearCandidates();

	bool horizontal = event->edit_candidates.horizontal;
	int num_candidates = event->edit_candidates.num_candidates;
	int selected_candidate = event->edit_candidates.selected_candidate;

	/* Calculate the length of the candidates text */
	size_t length = 0;
	for (i = 0; i < num_candidates; ++i)
	{
		if (horizontal)
		{
			if (i > 0)
			{
				++length;
			}
		}

		length += SDL_strlen(event->edit_candidates.candidates[i]);

		if (!horizontal)
		{
			length += 1;
		}
	}
	if (length == 0)
	{
		return;
	}
	++length; /* For null terminator */

	char* candidate_text = (char*)SDL_malloc(length);
	if (!candidate_text)
	{
		return;
	}

	char* dst = candidate_text;
	for (i = 0; i < num_candidates; ++i)
	{
		if (horizontal)
		{
			if (i > 0)
			{
				*dst++ = ' ';
			}
		}

		int length = (int)SDL_strlen(event->edit_candidates.candidates[i]);
		if (i == selected_candidate)
		{
			selected_candidate_start = (int)(uintptr_t)(dst - candidate_text);
			selected_candidate_length = length;
		}
		SDL_memcpy(dst, event->edit_candidates.candidates[i], length);
		dst += length;

		if (!horizontal)
		{
			*dst++ = '\n';
		}
	}
	*dst = '\0';

	candidates = TTF_CreateText(TTF_GetTextEngine(_pText), _pFont, candidate_text, 0);
	SDL_free(candidate_text);
	if (candidates)
	{
		float r, g, b, a;
		TTF_GetTextColorFloat(_pText, &r, &g, &b, &a);
		TTF_SetTextColorFloat(candidates, r, g, b, a);
	}
	else
	{
		ClearCandidates();
	}
}

void TextBox::DrawCandidates(SDL_Renderer* pRenderer)
{
	SDL_Rect safe_rect;
	SDL_FRect candidates_rect;
	int candidates_w;
	int candidates_h;
	float x, y;

	/* Position the candidate window */
	TTF_SubString cursor;
	int offset = composition_start;
	if (composition_cursor_length > 0)
	{
		// Place the candidates at the active clause
		offset += composition_cursor;
	}
	if (!TTF_GetTextSubString(_pText, offset, &cursor))
		return;

	SDL_GetRenderSafeArea(pRenderer, &safe_rect);
	TTF_GetTextSize(candidates, &candidates_w, &candidates_h);
	candidates_rect.x = _rect.x + cursor.rect.x;
	candidates_rect.y = _rect.y + cursor.rect.y + cursor.rect.h + 2.0f;
	candidates_rect.w = 1.0f + 2.0f + candidates_w + 2.0f + 1.0f;
	candidates_rect.h = 1.0f + 2.0f + candidates_h + 2.0f + 1.0f;
	if ((candidates_rect.x + candidates_rect.w) > safe_rect.w)
	{
		candidates_rect.x = (safe_rect.w - candidates_rect.w);
		if (candidates_rect.x < 0.0f)
		{
			candidates_rect.x = 0.0f;
		}
	}

	/* Draw the candidate background */
	SDL_SetRenderDrawColor(pRenderer, 0xAA, 0xAA, 0xAA, 0xFF);
	SDL_RenderFillRect(pRenderer, &candidates_rect);
	SDL_SetRenderDrawColor(pRenderer, 0x00, 0x00, 0x00, 0xFF);
	SDL_RenderRect(pRenderer, &candidates_rect);

	/* Draw the candidates */
	x = candidates_rect.x + 3.0f;
	y = candidates_rect.y + 3.0f;
	DrawText(pRenderer, candidates, x, y);

	/* Underline the selected candidate */
	if (selected_candidate_length > 0)
	{
		int font_height = TTF_GetFontHeight(_pFont);
		TTF_SubString** substrings = TTF_GetTextSubStringsForRange(candidates, selected_candidate_start, selected_candidate_length, NULL);
		if (substrings)
		{
			for (int i = 0; substrings[i]; ++i)
			{
				SDL_FRect rect;
				SDL_RectToFRect(&substrings[i]->rect, &rect);
				rect.x += x;
				rect.y += (y + font_height);
				rect.h = 1.0f;
				SDL_RenderFillRect(pRenderer, &rect);
			}
			SDL_free(substrings);
		}
	}
}

void TextBox::UpdateTextInputArea()
{
	SDL_Renderer* pRenderer = Application::GetRenderer();
	SDL_Window* pWindow = Application::GetWindow();

	/* Convert the text input area and cursor into window coordinates */
	SDL_FPoint window_edit_rect_min;
	SDL_FPoint window_edit_rect_max;
	SDL_FPoint window_cursor;
	if (!SDL_RenderCoordinatesToWindow(pRenderer, _rect.x, _rect.y, &window_edit_rect_min.x, &window_edit_rect_min.y) ||
		!SDL_RenderCoordinatesToWindow(pRenderer, _rect.x + _rect.w, _rect.y + _rect.h, &window_edit_rect_max.x, &window_edit_rect_max.y) ||
		!SDL_RenderCoordinatesToWindow(pRenderer, _cursor_rect.x, _cursor_rect.y, &window_cursor.x, &window_cursor.y))
	{
		return;
	}

	SDL_Rect rect;
	rect.x = (int)SDL_roundf(window_edit_rect_min.x);
	rect.y = (int)SDL_roundf(window_edit_rect_min.y);
	rect.w = (int)SDL_roundf(window_edit_rect_max.x - window_edit_rect_min.x);
	rect.h = (int)SDL_roundf(window_edit_rect_max.y - window_edit_rect_min.y);
	int cursor_offset = (int)SDL_roundf(window_cursor.x - window_edit_rect_min.x);
	SDL_SetTextInputArea(pWindow, &rect, cursor_offset);
}

void TextBox::DrawCursor(SDL_Renderer* pRenderer)
{
	if (composition_length > 0)
	{
		DrawCompositionCursor(pRenderer);
		return;
	}
	
	SDL_SetRenderDrawColor(pRenderer, 0, 0, 0, 0xFF);
	SDL_RenderFillRect(pRenderer, &_cursor_rect);
}

void TextBox::SetFocus(bool focus)
{
	if (_bFocused == focus)
		return;

	_bFocused = focus;
	SDL_Window* pWindow = Application::GetWindow();
	_cursor_visible = true;
	_last_cursor_change = SDL_GetTicks();

	if (_bFocused)
		SDL_StartTextInput(pWindow);
	else
		SDL_StopTextInput(pWindow);
}

static int GetCursorTextIndex(int x, const TTF_SubString* substring)
{
	if (substring->flags & (TTF_SUBSTRING_LINE_END | TTF_SUBSTRING_TEXT_END))
	{
		return substring->offset;
	}

	bool round_down = (x < (substring->rect.x + substring->rect.w / 2));

	if (round_down)
	{
		/* Start the cursor before the selected text */
		return substring->offset;
	}
	else
	{
		/* Place the cursor after the selected text */
		return substring->offset + substring->length;
	}
}

void TextBox::SetCursorPosition(int position)
{
	if (composition_length > 0)
	{
		/* Don't let the cursor be moved into the composition */
		if (position >= composition_start && position <= (composition_start + composition_length))
			return;

		CancelComposition();
	}

	_cursor = position;
}

void TextBox::MoveCursorIndex(int direction)
{
	TTF_SubString substring;

	int last_cursor = _cursor;

	if (direction < 0)
	{
		if (TTF_GetTextSubString(_pText, _cursor - 1, &substring))
		{
			SetCursorPosition(substring.offset);
		}
	}
	else
	{
		if (TTF_GetTextSubString(_pText, _cursor, &substring) &&
			TTF_GetTextSubString(_pText, substring.offset + SDL_max(substring.length, 1), &substring))
		{
			SetCursorPosition(substring.offset);
		}
	}

	OnMoveCursor(last_cursor);
}

void TextBox::OnMoveCursor(int last)
{
	bool isShiftDown = IsShiftDown();
	bool is_highlighting = highlight_start != -1 && highlight_end != -1;
	if (!is_highlighting && isShiftDown)
	{
		highlight_start = last;
		highlight_end = last;
		is_highlighting = true;
	}
	else if (is_highlighting && !isShiftDown)
	{
		highlight_start = -1;
		highlight_end = -1;
		is_highlighting = false;
	}

	if (is_highlighting)
	{
		int start = highlight_start;
		int end = highlight_end;
		int curr = _cursor;
		if (start == last)
			start = curr;
		else
			end = curr;
		
		highlight_start = SDL_min(start, end);
		highlight_end = SDL_max(start, end);
	}
}

void TextBox::MoveCursorLeft()
{
	MoveCursorIndex(-1);
}

void TextBox::MoveCursorRight()
{
	MoveCursorIndex(1);
}

void TextBox::MoveCursorUp()
{
	TTF_SubString substring;
	if (TTF_GetTextSubString(_pText, _cursor, &substring))
	{
		int fontHeight = TTF_GetFontHeight(_pFont);
		int x, y;
		x = substring.rect.x;
		y = substring.rect.y - fontHeight / 2;
		if (TTF_GetTextSubStringForPoint(_pText, x, y, &substring))
		{
			int last_cursor = _cursor;
			SetCursorPosition(GetCursorTextIndex(x, &substring));
			OnMoveCursor(last_cursor);
		}
	}
}

void TextBox::MoveCursorDown()
{
	TTF_SubString substring;
	if (TTF_GetTextSubString(_pText, _cursor, &substring))
	{
		int fontHeight = TTF_GetFontHeight(_pFont);
		int x, y;
		x = substring.rect.x;
		y = substring.rect.y + substring.rect.h + fontHeight / 2;
		if (TTF_GetTextSubStringForPoint(_pText, x, y, &substring))
		{
			int last_cursor = _cursor;
			SetCursorPosition(GetCursorTextIndex(x, &substring));
			OnMoveCursor(last_cursor);
		}
	}
}

void TextBox::MoveCursorBeginningOfLine()
{
	TTF_SubString substring;
	if (TTF_GetTextSubString(_pText, _cursor, &substring) &&
		TTF_GetTextSubStringForLine(_pText, substring.line_index, &substring))
	{
		int last_cursor = _cursor;
		SetCursorPosition(substring.offset);
		OnMoveCursor(last_cursor);
	}
}

void TextBox::MoveCursorEndOfLine()
{
	TTF_SubString substring;
	if (TTF_GetTextSubString(_pText, _cursor, &substring) &&
		TTF_GetTextSubStringForLine(_pText, substring.line_index, &substring))
	{
		int last_cursor = _cursor;
		SetCursorPosition(substring.offset + substring.length);
		OnMoveCursor(last_cursor);
	}
}

void TextBox::MoveCursorBeginning()
{
	/* Move to the beginning of the text */
	SetCursorPosition(0);
}

void TextBox::MoveCursorEnd()
{
	/* Move to the end of the text */
	if (_pText->text)
	{
		SetCursorPosition((int)SDL_strlen(_pText->text));
	}
}

void TextBox::Backspace()
{
	if (!_pText->text)
		return;

	if (DeleteHighlight())
		return;

	if (_cursor > 0)
	{
		const char* start = &_pText->text[_cursor];
		const char* next = start;
		SDL_StepBackUTF8(_pText->text, &next);
		int length = (int)(uintptr_t)(start - next);
		TTF_DeleteTextString(_pText, _cursor - length, length);
		_cursor -= length;
	}
}

void TextBox::BackspaceToBeginning()
{
	/* Delete to the beginning of the string */
	TTF_DeleteTextString(_pText, 0, _cursor);
	SetCursorPosition(0);
}

void TextBox::DeleteToEnd()
{
	/* Delete to the end of the string */
	TTF_DeleteTextString(_pText, _cursor, -1);
}

void TextBox::Delete()
{
	if (!_pText->text)
		return;

	if (DeleteHighlight())
		return;

	const char* start = &_pText->text[_cursor];
	const char* next = start;
	size_t length = SDL_strlen(next);
	SDL_StepUTF8(&next, &length);
	length = (next - start);
	TTF_DeleteTextString(_pText, _cursor, (int)length);
}

bool TextBox::HandleMouseDown(float x, float y)
{
	SDL_FPoint pt = { x, y };
	if (!SDL_PointInRectFloat(&pt, &_rect))
	{
		if (_bFocused)
		{
			SetFocus(false);
			return true;
		}
		return false;
	}

	if (!_bFocused)
	{
		SetFocus(true);
	}

	/* Set the cursor position */
	TTF_SubString substring;
	int textX = (int)SDL_roundf(x - _rect.x);
	int textY = (int)SDL_roundf(y - _rect.y);
	if (!TTF_GetTextSubStringForPoint(_pText, textX, textY, &substring))
	{
		SDL_Log("Couldn't get cursor location: %s", SDL_GetError());
		return false;
	}

	SetCursorPosition(GetCursorTextIndex(textX, &substring));
	_bIsHighlighting = true;
	highlight_start = _cursor;
	highlight_end = -1;

	return true;
}

bool TextBox::HandleMouseMotion(float x, float y)
{
	if (!_bIsHighlighting)
		return false;

	/* Set the highlight position */
	TTF_SubString substring;
	int textX = (int)SDL_roundf(x - _rect.x);
	int textY = (int)SDL_roundf(y - _rect.y);
	if (!TTF_GetTextSubStringForPoint(_pText, textX, textY, &substring))
	{
		SDL_Log("Couldn't get cursor location: %s", SDL_GetError());
		return false;
	}

	SetCursorPosition(GetCursorTextIndex(textX, &substring));
	highlight_end = _cursor;

	return true;
}

bool TextBox::HandleMouseUp(float x, float y)
{
	if (!_bIsHighlighting)
		return false;

	_bIsHighlighting = false;
	return true;
}

void TextBox::SelectAll()
{
	if (!_pText->text)
		return;

	highlight_start = 0;
	highlight_end = (int)SDL_strlen(_pText->text);
}

bool TextBox::DeleteHighlight()
{
	if (!_pText->text)
		return false;

	int marker, length;
	if (GetHighlightExtents(&marker, &length))
	{
		TTF_DeleteTextString(_pText, marker, length);
		SetCursorPosition(marker);
		highlight_start = -1;
		highlight_end = -1;
		return true;
	}
	return false;
}

void TextBox::Copy()
{
	if (!_pText->text)
		return;

	int marker, length;
	if (GetHighlightExtents(&marker, &length))
	{
		char* temp = (char*)SDL_malloc(length + 1);
		if (temp)
		{
			SDL_memcpy(temp, &_pText->text[marker], length);
			temp[length] = '\0';
			SDL_SetClipboardText(temp);
			SDL_free(temp);
		}
	}
	else
	{
		SDL_SetClipboardText(_pText->text);
	}
}

void TextBox::Cut()
{
	if (!_pText->text)
		return;

	/* Copy to clipboard and delete text */
	int marker, length;
	if (GetHighlightExtents(&marker, &length))
	{
		char* temp = (char*)SDL_malloc(length + 1);
		if (temp)
		{
			SDL_memcpy(temp, &_pText->text[marker], length);
			temp[length] = '\0';
			SDL_SetClipboardText(temp);
			SDL_free(temp);
		}
		TTF_DeleteTextString(_pText, marker, length);
		SetCursorPosition(marker);
		highlight_start = -1;
		highlight_end = -1;
	}
	else
	{
		SDL_SetClipboardText(_pText->text);
		TTF_DeleteTextString(_pText, 0, -1);
	}
}

void TextBox::Paste()
{
	const char* text = SDL_GetClipboardText();
	Insert(text);
}

void TextBox::Insert(const char* text)
{
	if (!text)
		return;

	DeleteHighlight();

	if (composition_length > 0)
	{
		TTF_DeleteTextString(_pText, composition_start, composition_length);
		composition_length = 0;
	}

	size_t length = SDL_strlen(text);
	TTF_InsertTextString(_pText, _cursor, text, length);
	SetCursorPosition((int)(_cursor + length));
}

bool TextBox::OnEvent(SDL_Event* event)
{
	if (!event)
	{
		return false;
	}

	switch (event->type)
	{
	case SDL_EVENT_MOUSE_BUTTON_DOWN:
		return HandleMouseDown(event->button.x, event->button.y);

	case SDL_EVENT_MOUSE_MOTION:
		return HandleMouseMotion(event->motion.x, event->motion.y);

	case SDL_EVENT_MOUSE_BUTTON_UP:
		return HandleMouseUp(event->button.x, event->button.y);

	case SDL_EVENT_KEY_DOWN:
		if (!_bFocused)
		{
			break;
		}

		switch (event->key.key)
		{
		case SDLK_A:
			if (event->key.mod & SDL_KMOD_CTRL)
			{
				SelectAll();
			}
			break;
		case SDLK_C:
			if (event->key.mod & SDL_KMOD_CTRL)
			{
				Copy();
			}
			break;

		case SDLK_V:
			if (event->key.mod & SDL_KMOD_CTRL)
			{
				Paste();
			}
			break;

		case SDLK_X:
			if (event->key.mod & SDL_KMOD_CTRL)
			{
				Cut();
			}
			break;

		case SDLK_LEFT:
			if (event->key.mod & SDL_KMOD_CTRL)
			{
				MoveCursorBeginningOfLine();
			}
			else
			{
				MoveCursorLeft();
			}
			break;

		case SDLK_RIGHT:
			if (event->key.mod & SDL_KMOD_CTRL)
			{
				MoveCursorEndOfLine();
			}
			else
			{
				MoveCursorRight();
			}
			break;

		case SDLK_UP:
			if (event->key.mod & SDL_KMOD_CTRL)
			{
				MoveCursorBeginning();
			}
			else
			{
				MoveCursorUp();
			}
			break;

		case SDLK_DOWN:
			if (event->key.mod & SDL_KMOD_CTRL)
			{
				MoveCursorEnd();
			}
			else
			{
				MoveCursorDown();
			}
			break;

		case SDLK_HOME:
			MoveCursorBeginningOfLine();
			break;

		case SDLK_END:
			MoveCursorEndOfLine();
			break;

		case SDLK_BACKSPACE:
			if (event->key.mod & SDL_KMOD_CTRL)
			{
				BackspaceToBeginning();
			}
			else
			{
				Backspace();
			}
			break;

		case SDLK_DELETE:
			if (event->key.mod & SDL_KMOD_CTRL)
			{
				DeleteToEnd();
			}
			else
			{
				Delete();
			}
			break;

		case SDLK_RETURN:
			Insert("\n");
			break;

		case SDLK_ESCAPE:
			SetFocus(false);
			break;

		default:
			break;
		}
		return true;

	case SDL_EVENT_TEXT_INPUT:
		Insert(event->text.text);
		return true;

	case SDL_EVENT_TEXT_EDITING:
		HandleComposition(&event->edit);
		break;

	case SDL_EVENT_TEXT_EDITING_CANDIDATES:
		ClearCandidates();
		SaveCandidates(event);
		break;

	default:
		break;
	}
	return false;
}
