#include <pch.h>
#include "gui/TextInput2.h"
#include "gui/Window.h"
#include "app/AppState.h"
#include <algorithm>

constexpr uint64_t CursorBlinkIntervalMS { 500ULL };

static int UTF8ByteLength(const char* text, int num_codepoints)
{
	if (!text)
		return 0;
	const char* start = text;
	while (num_codepoints > 0)
	{
		if (SDL_StepUTF8(&text, NULL) == 0)
			break;
		--num_codepoints;
	}
	return (int)(uintptr_t)(text - start);
}

static int BytesUTF8Length(const char* text, int num_bytes)
{
	if (!text)
		return 0;
	const char* start = text;
	const char* end = text + num_bytes;
	int num_codepoints = 0;
	while (text < end)
	{
		++num_codepoints;
		Uint32 ch = SDL_StepUTF8(&text, NULL);
		if (ch == 0)
			break;
	}
	return num_codepoints;
}

namespace fig::gui
{
	TextInput2::TextInput2(ControlPtr pParent, FontFace fontFace, double ptSize, TextInput2::Flags flags) : Control(pParent),
		_flags { flags }
	{
		SetForegroundColor(Color::TextBoxForeground);
		SetBackgroundColor(Color::TextBoxBackground);

		_pFont = Fonts::GetFont(fontFace, ptSize);
		_pPlaceholder = TTF_CreateText(GetSDLTextEngine(), _pFont, nullptr, 0);
		TTF_SetTextWrapWhitespaceVisible(_pPlaceholder, true);
		_body = TTFTextBody(GetSDLTextEngine(), _pFont);

		if (flags.IsSet(Flag::Password))
			_pPassword = TTF_CreateText(GetSDLTextEngine(), _pFont, nullptr, 0);

		_bFocused = false;
		EnableClipping(false);

		highlight_start = -1;
		highlight_end = -1;

		if (_pFont)
			SetSize(300, MeasureFontHeight(*_pFont) + GetMarginVertical());
	}

	TextInput2::~TextInput2()
	{
		ClearCandidates();
		TTF_DestroyText(_pPassword);
		TTF_DestroyText(_pPlaceholder);
	}

	TTF_Text* TextInput2::GetRenderedText()
	{
		return _flags.IsSet(Flag::Password) && _pPassword ? _pPassword : _pText;
	}

	void TextInput2::OnUpdate(float fElapsed)
	{
		uint64_t now = SDL_GetTicks();
		if ((now - _last_cursor_change) >= CursorBlinkIntervalMS)
		{
			_cursor_visible = !_cursor_visible;
			_last_cursor_change = now;
		}

		UpdatePassword();

		if (_bFocused)
		{
			int32_t cursor_pos = ConvertToPasswordPosition(_body.GetCursorPosition());

			/* Calculate the cursor rect, used for positioning candidates */
			TTF_SubString cursor;
			if (TTF_GetTextSubString(GetRenderedText(), cursor_pos, &cursor))
			{
				auto& rect = GetRect();
				fig::rectf cursor_rect = to_rectf(cursor.rect);
				cursor_rect.x += rect.x + GetMarginLeft();
				cursor_rect.y += rect.y + GetMarginTop();
				cursor_rect.w = 1.0f;
				cursor_rect.h = std::max(cursor_rect.h, (float)TTF_GetFontLineSkip(_pFont));
				SDL_copyp(&_cursor_rect, &cursor_rect);

				UpdateTextInputArea();
			}
		}
	}

	void TextInput2::OnRender(fig::renderer_ptr pRenderer)
	{
		DrawBackground(pRenderer);
		DrawBorder(pRenderer);

		int lineSkip = TTF_GetFontLineSkip(_pFont);
		int maxRows = IsMultiline() ? std::max(_maxRows, 1) : 1;

		auto& rect = GetRect();
		auto clientRect = GetClientRect();

		// Clipping
		fig::rect prevClippingRect;
		bool restoreClipping = SDL_GetRenderClipRect(pRenderer, &prevClippingRect) && prevClippingRect.w > 0;
		fig::rect clippingRect = clientRect;
		clippingRect.h = std::min(clippingRect.h, lineSkip * maxRows);
		SDL_SetRenderClipRect(pRenderer, &clippingRect);

		// Scroll to cursor
		if (_bFocused)
		{
			if (IsMultiline()) // Multiline (Vertical scrolling)
			{
				float cursorY = _cursor_rect.y - clientRect.y;
				while (toI(std::round((cursorY - _scroll.y) / lineSkip)) >= maxRows)
					_scroll.y += lineSkip;
				while (toI(std::round((cursorY - _scroll.y) / lineSkip)) < 0)
					_scroll.y -= lineSkip;
				_scroll.x = 0;
			}
			else // Single line (Horizontal scrolling)
			{
				constexpr int32_t kScrollStep = 80;

				int maxCursorX = clientRect.w;
				int textWidth, _;
				TTF_GetTextSize(GetRenderedText(), &textWidth, &_);
				int cursorX = toI(_cursor_rect.x + _cursor_rect.w - clientRect.x);
				while (cursorX > 0 and cursorX - _scroll.x > maxCursorX)
					_scroll.x = std::min(_scroll.x + kScrollStep, cursorX - maxCursorX);
				while (cursorX > 0 and cursorX - _scroll.x < 0)
					_scroll.x = std::max(_scroll.x - kScrollStep, 0);
				_scroll.y = 0;
			}
		}

		// Draw highlight(s) 
		int marker, length;
		if (GetHighlightExtents(&marker, &length))
		{
			if (IsPassword())
			{
				int utf8_text_start = BytesUTF8Length(_body.GetText().c_str(), marker);
				int utf8_text_end = BytesUTF8Length(_body.GetText().c_str(), marker + length);
				marker = UTF8ByteLength(_pPassword->text, utf8_text_start);
				length = UTF8ByteLength(_pPassword->text, utf8_text_end) - marker;
			}

			TTF_SubString** pHighlights = TTF_GetTextSubStringsForRange(GetRenderedText(), marker, length, NULL);
			if (pHighlights)
			{
				SDL_SetRenderDrawColor(pRenderer, Color::TextSelectionBackground.r, Color::TextSelectionBackground.g, Color::TextSelectionBackground.b, Color::TextSelectionBackground.a);
				for (int i = 0; pHighlights[i]; ++i)
				{
					fig::rectf highlight_rect = to_rectf(pHighlights[i]->rect);
					highlight_rect.w = std::max(highlight_rect.w, 3.0f);
					highlight_rect.x += rect.x + GetMarginLeft();
					highlight_rect.y += rect.y + GetMarginTop();

					ApplyScroll(highlight_rect);
					SDL_RenderFillRect(pRenderer, &highlight_rect);
				}
				SDL_free(pHighlights);
			}
		}

		if (auto pText = GetRenderedText(); pText->text)
			DrawText(pRenderer, pText, rect.x, rect.y + 8);
		else
			DrawPlaceholder(pRenderer, rect.x, rect.y + 8);

		if (_bFocused)
		{
			if (composition_length > 0)
				DrawComposition(pRenderer);

			if (candidates)
				DrawCandidates(pRenderer);

			if (_cursor_visible)
				DrawCursor(pRenderer);
		}

		SDL_SetRenderClipRect(pRenderer, restoreClipping ? &prevClippingRect : nullptr);


		_body.Render(pRenderer); //! @temp
	}

	void TextInput2::DrawText(fig::renderer_ptr pRenderer, TTF_Text* pText, int x, int y)
	{
		auto fgColor = GetForegroundColor();
		TTF_SetTextColor(pText, fgColor.r, fgColor.g, fgColor.b, fgColor.a);

		auto& rect = GetRect();
		int xx = rect.x + GetMarginLeft();
		int yy = rect.y + GetMarginTop();
		ApplyScroll(xx, yy);

		TTF_DrawRendererText(pText, toF(xx), toF(yy));
	}

	void TextInput2::DrawPlaceholder(fig::renderer_ptr pRenderer, int x, int y)
	{
		if (!_pPlaceholder->text)
			return;

		auto fgColor = Color::DisabledForeground;
		TTF_SetTextColor(_pPlaceholder, fgColor.r, fgColor.g, fgColor.b, fgColor.a);

		auto& rect = GetRect();
		int xx = rect.x + GetMarginLeft();
		int yy = rect.y + GetMarginTop();
		ApplyScroll(xx, yy);

		TTF_DrawRendererText(_pPlaceholder, toF(xx), toF(yy));
	}

	void TextInput2::DrawCursor(fig::renderer_ptr pRenderer)
	{
		if (composition_length > 0)
		{
			DrawCompositionCursor(pRenderer);
			return;
		}

		auto rect = _cursor_rect;
		ApplyScroll(rect);
		SDL_SetRenderDrawColor(pRenderer, 0, 0, 0, 0xFF);
		SDL_RenderFillRect(pRenderer, &rect);
	}

	bool TextInput2::GetHighlightExtents(int* marker, int* length)
	{
		if (highlight_start >= 0 and highlight_end >= 0)
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

	static bool IsShiftDown()
	{
		return (SDL_GetModState() & SDL_KMOD_SHIFT) != 0;
	}

	void TextInput2::ResetCursorBlink()
	{
		_cursor_visible = true;
		_last_cursor_change = SDL_GetTicks();
	}

#pragma region Composition

	void TextInput2::ResetComposition()
	{
		composition_start = 0;
		composition_length = 0;
		composition_cursor = 0;
		composition_cursor_length = 0;
	}

	void TextInput2::HandleComposition(const SDL_TextEditingEvent* event)
	{
//		DeleteHighlight(); //! @todo
//
//		if (composition_length > 0)
//		{
//			TTF_DeleteTextString(_pText, composition_start, composition_length);
//			ResetComposition();
//		}
//
//		int length = (int)SDL_strlen(event->text);
//		if (length > 0)
//		{
//			composition_start = _cursor;
//			composition_length = length;
//			TTF_InsertTextString(_pText, composition_start, event->text, composition_length);
//			if (event->start > 0 or event->length > 0)
//			{
//				composition_cursor = UTF8ByteLength(&_pText->text[composition_start], event->start);
//				composition_cursor_length = UTF8ByteLength(&_pText->text[composition_start + composition_cursor], event->length);
//			}
//			else
//			{
//				composition_cursor = length;
//				composition_cursor_length = 0;
//			}
//			PushUndo(UndoAction::Write);
//			DidChange();
//		}
	}

	void TextInput2::CancelComposition()
	{
		ResetComposition();

		SDL_ClearComposition(GetSDLWindow());
	}

	void TextInput2::DrawComposition(fig::renderer_ptr pRenderer)
	{
		//! @todo
//		/* Draw an underline under the composed text */
//		int font_height = TTF_GetFontHeight(_pFont);
//		TTF_SubString** substrings = TTF_GetTextSubStringsForRange(_pText, composition_start, composition_length, NULL);
//		if (substrings)
//		{
//			for (int i = 0; substrings[i]; ++i)
//			{
//				fig::rectf rect;
//				SDL_RectToFRect(&substrings[i]->rect, &rect);
//				rect.x += rect.x;
//				rect.y += (rect.y + font_height);
//				rect.h = 1.0f;
//				SDL_RenderFillRect(pRenderer, &rect);
//			}
//			SDL_free(substrings);
//		}
//
//		/* Thicken the underline under the active clause in the composed text */
//		if (composition_cursor_length > 0)
//		{
//			substrings = TTF_GetTextSubStringsForRange(_pText, composition_start + composition_cursor, composition_cursor_length, NULL);
//			if (substrings)
//			{
//				for (int i = 0; substrings[i]; ++i)
//				{
//					fig::rectf rect;
//					SDL_RectToFRect(&substrings[i]->rect, &rect);
//					rect.x += rect.x;
//					rect.y += (rect.y + font_height) - 1;
//					rect.h = 8.0f;
//					SDL_RenderFillRect(pRenderer, &rect);
//				}
//				SDL_free(substrings);
//			}
//		}
	}

	void TextInput2::DrawCompositionCursor(fig::renderer_ptr pRenderer)
	{
		//! @todo
//		if (composition_cursor_length == 0)
//		{
//			TTF_SubString cursor;
//			if (TTF_GetTextSubString(_pText, composition_start + composition_cursor, &cursor))
//			{
//				fig::rectf rect = to_rectf(cursor.rect);
//				rect.x += rect.x;
//				rect.y += rect.y;
//				rect.w = 1.0f;
//
//				ApplyScroll(rect);
//				SDL_SetRenderDrawColor(pRenderer, 0, 0, 0, 0xFF);
//				SDL_RenderFillRect(pRenderer, &rect);
//			}
//		}
	}

	void TextInput2::ClearCandidates()
	{
		if (candidates)
		{
			TTF_DestroyText(candidates);
			candidates = NULL;
		}
		selected_candidate_start = 0;
		selected_candidate_length = 0;
	}

	void TextInput2::SaveCandidates(const SDL_Event* event)
	{
//		int i;
//
//		ClearCandidates();
//
//		bool horizontal = event->edit_candidates.horizontal;
//		int num_candidates = event->edit_candidates.num_candidates;
//		int selected_candidate = event->edit_candidates.selected_candidate;
//
//		/* Calculate the length of the candidates text */
//		size_t length = 0;
//		for (i = 0; i < num_candidates; ++i)
//		{
//			if (horizontal)
//			{
//				if (i > 0)
//				{
//					++length;
//				}
//			}
//
//			length += SDL_strlen(event->edit_candidates.candidates[i]);
//
//			if (!horizontal)
//			{
//				length += 1;
//			}
//		}
//		if (length == 0)
//		{
//			return;
//		}
//		++length; /* For null terminator */
//
//		char* candidate_text = (char*)SDL_malloc(length);
//		if (!candidate_text)
//		{
//			return;
//		}
//
//		char* dst = candidate_text;
//		for (i = 0; i < num_candidates; ++i)
//		{
//			if (horizontal)
//			{
//				if (i > 0)
//				{
//					*dst++ = ' ';
//				}
//			}
//
//			int length = (int)SDL_strlen(event->edit_candidates.candidates[i]);
//			if (i == selected_candidate)
//			{
//				selected_candidate_start = (int)(uintptr_t)(dst - candidate_text);
//				selected_candidate_length = length;
//			}
//			SDL_memcpy(dst, event->edit_candidates.candidates[i], length);
//			dst += length;
//
//			if (!horizontal)
//			{
//				*dst++ = '\n';
//			}
//		}
//		*dst = '\0';
//
//		candidates = TTF_CreateText(GetSDLTextEngine(), _pFont, candidate_text, 0);
//		SDL_free(candidate_text);
//		if (candidates)
//		{
//			float r, g, b, a;
//			TTF_GetTextColorFloat(_pText, &r, &g, &b, &a);
//			TTF_SetTextColorFloat(candidates, r, g, b, a);
//		}
//		else
//		{
//			ClearCandidates();
//		}
	}

	void TextInput2::DrawCandidates(fig::renderer_ptr pRenderer)
	{
//		SDL_Rect safe_rect;
//		fig::rectf candidates_rect;
//		int candidates_w;
//		int candidates_h;
//
//		auto& rect = GetRect();
//
//		/* Position the candidate window */
//		TTF_SubString cursor;
//		int offset = composition_start;
//		if (composition_cursor_length > 0)
//		{
//			// Place the candidates at the active clause
//			offset += composition_cursor;
//		}
//		if (!TTF_GetTextSubString(_pText, offset, &cursor))
//			return;
//
//		SDL_GetRenderSafeArea(pRenderer, &safe_rect);
//		TTF_GetTextSize(candidates, &candidates_w, &candidates_h);
//		candidates_rect.x = toF(rect.x + GetMarginLeft() + cursor.rect.x);
//		candidates_rect.y = toF(rect.y + GetMarginTop() + cursor.rect.y + cursor.rect.h + 2);
//		candidates_rect.w = 1.0f + 2.0f + candidates_w + 2.0f + 1.0f;
//		candidates_rect.h = 1.0f + 2.0f + candidates_h + 2.0f + 1.0f;
//		if ((candidates_rect.x + candidates_rect.w) > safe_rect.w)
//		{
//			candidates_rect.x = (safe_rect.w - candidates_rect.w);
//			if (candidates_rect.x < 0.0f)
//			{
//				candidates_rect.x = 0.0f;
//			}
//		}
//
//		ApplyScroll(candidates_rect);
//
//		/* Draw the candidate background */
//		SDL_SetRenderDrawColor(pRenderer, 0xAA, 0xAA, 0xAA, 0xFF);
//		SDL_RenderFillRect(pRenderer, &candidates_rect);
//		SDL_SetRenderDrawColor(pRenderer, 0x00, 0x00, 0x00, 0xFF);
//		SDL_RenderRect(pRenderer, &candidates_rect);
//
//		/* Draw the candidates */
//		int x = toI(candidates_rect.x + 3);
//		int y = toI(candidates_rect.y + 3);
//		DrawText(pRenderer, candidates, x, y);
//
//		/* Underline the selected candidate */
//		if (selected_candidate_length > 0)
//		{
//			int font_height = TTF_GetFontHeight(_pFont);
//			TTF_SubString** substrings = TTF_GetTextSubStringsForRange(candidates, selected_candidate_start, selected_candidate_length, NULL);
//			if (substrings)
//			{
//				for (int i = 0; substrings[i]; ++i)
//				{
//					fig::rectf rect;
//					SDL_RectToFRect(&substrings[i]->rect, &rect);
//					rect.x += x;
//					rect.y += (y + font_height);
//					rect.h = 1.0f;
//					SDL_RenderFillRect(pRenderer, &rect);
//				}
//				SDL_free(substrings);
//			}
//		}
	}

#pragma endregion Composition

	void TextInput2::UpdateTextInputArea()
	{
		SDL_Window* pWindow = GetSDLWindow();
		fig::renderer_ptr pRenderer = GetSDLRenderer();
		auto& rect = GetRect();

		/* Convert the text input area and cursor into window coordinates */
		fig::pointf window_edit_rect_min;
		fig::pointf window_edit_rect_max;
		fig::pointf window_cursor;
		if (!SDL_RenderCoordinatesToWindow(pRenderer, toF(rect.x + GetMarginLeft()), toF(rect.y + GetMarginTop()), &window_edit_rect_min.x, &window_edit_rect_min.y) or
			!SDL_RenderCoordinatesToWindow(pRenderer, toF(rect.x + GetMarginLeft() + rect.w), toF(rect.y + GetMarginTop() + rect.h), &window_edit_rect_max.x, &window_edit_rect_max.y) or
			!SDL_RenderCoordinatesToWindow(pRenderer, toF(_cursor_rect.x), toF(_cursor_rect.y), &window_cursor.x, &window_cursor.y))
		{
			return;
		}

		SDL_Rect input_rect;
		input_rect.x = (int)SDL_roundf(window_edit_rect_min.x);
		input_rect.y = (int)SDL_roundf(window_edit_rect_min.y);
		input_rect.w = (int)SDL_roundf(window_edit_rect_max.x - window_edit_rect_min.x);
		input_rect.h = (int)SDL_roundf(window_edit_rect_max.y - window_edit_rect_min.y);
		int cursor_offset = (int)SDL_roundf(window_cursor.x - window_edit_rect_min.x);
		SDL_SetTextInputArea(pWindow, &input_rect, cursor_offset);
	}

	void TextInput2::SetFocus(bool bFocus)
	{
		bFocus &= GetEnabled();
		if (_bFocused == bFocus)
			return;

		_bFocused = bFocus;
		_cursor_visible = bFocus;
		_last_cursor_change = SDL_GetTicks();

		if (_bFocused)
		{
			PushEvent(UserEvent::StartTextInput, 0, this);
		}
		else
		{
			PushEvent(UserEvent::StopTextInput, 0, this);
		}
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

	static int FindPriorWord(TTF_Text* pText, int cursor)
	{
		const char* start = &pText->text[cursor];
		const char* zero = &pText->text[0];
		const char* curr = start;
		if (start == zero)
			return 0;

		enum CharType {
			Whitespace,
			Punctuation,
			Character,
		};

		auto fnCharType = [](char ch) -> CharType {
			if (is_whitespace(ch)) return CharType::Whitespace;
			if (is_punctuation(ch)) return CharType::Punctuation;
			return CharType::Character;
		};

		auto fnMoveLeft = [pText, &fnCharType](const char*& ch) -> CharType {
			SDL_StepBackUTF8(pText->text, &ch);
			return fnCharType(ch[0]);
		};

		auto fnMoveRight = [pText, &fnCharType](const char*& ch) -> CharType {
			size_t length = SDL_strlen(ch);
			SDL_StepUTF8(&ch, &length);
			return fnCharType(ch[0]);
		};

		// Skip any whitespace first
		while (curr > zero)
		{
			CharType charType = fnMoveLeft(curr);
			if (not is_whitespace(curr[0]))
			{
				fnMoveRight(curr);
				break;
			}
		}

		// Find first position of word
		if (curr > zero)
		{
			CharType skipType = fnMoveLeft(curr);
			while (curr > zero)
			{
				CharType priorType = fnMoveLeft(curr);
				if (priorType == skipType)
					continue;

				fnMoveRight(curr);
				break;
			}
		}

		int length = (int)(uintptr_t)(start - curr);
		return cursor - length;
	}

	static int FindNextWord(TTF_Text* pText, int cursor)
	{
		size_t length = SDL_strlen(pText->text);
		const char* start = &pText->text[cursor];
		const char* end = &pText->text[length];
		const char* curr = start;
		if (start == end)
			return cursor;

		enum CharType {
			Whitespace,
			Punctuation,
			Character,
		};

		auto fnCharType = [](char ch) -> CharType {
			if (is_whitespace(ch)) return CharType::Whitespace;
			if (is_punctuation(ch)) return CharType::Punctuation;
			return CharType::Character;
		};

		auto fnMoveLeft = [pText, &fnCharType](const char*& ch) -> CharType {
			SDL_StepBackUTF8(pText->text, &ch);
			return fnCharType(ch[0]);
		};

		auto fnMoveRight = [pText, &fnCharType](const char*& ch) -> CharType {
			size_t length = SDL_strlen(ch);
			SDL_StepUTF8(&ch, &length);
			return fnCharType(ch[0]);
		};

		CharType startType = fnCharType(start[0]);
		if (startType == CharType::Whitespace)
		{
			// On whitespace: only erase whitespace
			while (curr < end and is_whitespace(curr[0]))
				fnMoveRight(curr);
			int length = (int)(uintptr_t)(curr - start);
			return cursor + length;
		}
		else
		{
			// Otherwise: skip of same type
			CharType nextType = startType;
			while (curr < end and nextType == startType)
				nextType = fnMoveRight(curr);

			// ..then skip any whitespace
			while (curr < end and nextType == CharType::Whitespace)
				nextType = fnMoveRight(curr);

			int length = (int)(uintptr_t)(curr - start);
			return cursor + length;
		}
	}

	void TextInput2::SetCursorPosition(int32_t position)
	{
		if (composition_length > 0)
		{
			/* Don't let the cursor be moved into the composition */
			if (position >= composition_start and position <= (composition_start + composition_length))
				return;

			CancelComposition();
		}

		_body.SetCursor(position);
		ResetCursorBlink();
	}

	void TextInput2::MoveCursorIndex(int32_t direction)
	{
		auto last_cursor = _body.GetCursor();
		_body.MoveCursor(direction);
		OnMoveCursor(last_cursor.position);
	}

	void TextInput2::OnMoveCursor(int32_t last_position)
	{
		bool isShiftDown = IsShiftDown();
		bool is_highlighting = highlight_start != -1 and highlight_end != -1;
		if (!is_highlighting and isShiftDown)
		{
			highlight_start = last_position;
			highlight_end = last_position;
			is_highlighting = true;
		}
		else if (is_highlighting and !isShiftDown)
		{
			highlight_start = -1;
			highlight_end = -1;
			is_highlighting = false;
		}

		if (is_highlighting)
		{
			int start = highlight_start;
			int end = highlight_end;
			int curr = _body.GetCursorPosition();
			if (start == last_position)
				start = curr;
			else
				end = curr;

			highlight_start = SDL_min(start, end);
			highlight_end = SDL_max(start, end);
		}

		ResetCursorBlink();
	}

	void TextInput2::MoveCursorLeft()
	{
		MoveCursorIndex(-1);
	}

	void TextInput2::MoveCursorRight()
	{
		MoveCursorIndex(1);
	}

	void TextInput2::MoveCursorUp()
	{
		if (not IsMultiline())
			return;

		auto last_cursor = _body.GetCursor();
		_body.MoveCursorUp();
		OnMoveCursor(last_cursor.position);
	}

	void TextInput2::MoveCursorDown()
	{
		if (not IsMultiline())
			return;

		auto last_cursor = _body.GetCursor();
		_body.MoveCursorDown();
		OnMoveCursor(last_cursor.position);
	}

	void TextInput2::MoveCursorBeginningOfLine()
	{
		auto last_cursor = _body.GetCursor();
		_body.MoveCursorBeginningOfLine();
		OnMoveCursor(last_cursor.position);
	}

	void TextInput2::MoveCursorEndOfLine()
	{
		auto last_cursor = _body.GetCursor();
		_body.MoveCursorEndOfLine();
		OnMoveCursor(last_cursor.position);
	}

	void TextInput2::MoveCursorBeginning()
	{
		auto last_cursor = _body.GetCursor();
		_body.SetCursor(0);
		OnMoveCursor(last_cursor.position);
	}

	void TextInput2::MoveCursorEnd()
	{
		auto last_cursor = _body.GetCursor();
		_body.SetCursor(_body.GetText().length());
		OnMoveCursor(last_cursor.position);
	}

	void TextInput2::MoveCursorToPriorWord()
	{
		auto last_cursor = _body.GetCursor();
		_body.MoveCursorToPriorWord();
		OnMoveCursor(last_cursor.position);
	}

	void TextInput2::MoveCursorToNextWord()
	{
		auto last_cursor = _body.GetCursor();
		_body.MoveCursorToNextWord();
		OnMoveCursor(last_cursor.position);
	}

	void TextInput2::Backspace()
	{
		if (DeleteHighlight())
			return;

		if (_body.Backspace())
		{
			ResetCursorBlink();
			PushUndo(UndoAction::Erase);
			DidChange();
		}
	}

	void TextInput2::BackspaceToPriorWord()
	{
		if (DeleteHighlight())
			return;

		if (_body.BackspaceToPriorWord())
		{
			ResetCursorBlink();
			PushUndo(UndoAction::Erase, false);
			DidChange();
		}
	}

	void TextInput2::BackspaceToBeginning()
	{
		if (DeleteHighlight())
			return;

		if (_body.BackspaceToBeginning())
		{
			ResetCursorBlink();
			PushUndo(UndoAction::Erase, false);
			DidChange();
		}
	}

	void TextInput2::BackspaceToBeginningOfLine()
	{
		if (DeleteHighlight())
			return;

		if (_body.BackspaceToBeginningOfLine())
		{
			ResetCursorBlink();
			PushUndo(UndoAction::Erase, false);
			DidChange();
		}
	}

	void TextInput2::Delete()
	{
		if (DeleteHighlight())
			return;

		if (_body.Delete())
		{
			ResetCursorBlink();
			PushUndo(UndoAction::Erase);
			DidChange();
		}
	}

	void TextInput2::DeleteToNextWord()
	{
		if (DeleteHighlight())
			return;

		if (_body.DeleteToNextWord())
		{
			ResetCursorBlink();
			PushUndo(UndoAction::Erase, false);
			DidChange();
		}
	}

	void TextInput2::DeleteToEndOfLine()
	{
		if (DeleteHighlight())
			return;

		if (_body.DeleteToEndOfLine())
		{
			ResetCursorBlink();
			PushUndo(UndoAction::Erase, false);
			DidChange();
		}
	}

	void TextInput2::DeleteToEnd()
	{
		if (DeleteHighlight())
			return;

		if (_body.DeleteToEnd())
		{
			ResetCursorBlink();
			PushUndo(UndoAction::Erase, false);
			DidChange();
		}
	}

	bool TextInput2::DeleteHighlight()
	{
		if (_body.DeleteSelection())
		{
			PushUndo(UndoAction::Erase);
			DidChange();
			return true;
		}
		return false;
	}

#pragma region Selection
	bool TextInput2::HandleMouseDown(int x, int y)
	{
		fig::point pt = { x, y };
		auto rect = GetClientRect();
		if (!SDL_PointInRect(&pt, &rect))
		{
			if (_bFocused)
				SetFocus(false);
			return false;
		}

		if (!_bFocused)
			SetFocus(true);

		TTF_SubString substring;
		int textX = x - rect.x + _scroll.x;
		int textY = y - rect.y + _scroll.y;
		if (TTF_GetTextSubStringForPoint(GetRenderedText(), textX, textY, &substring))
		{
			int32_t pos = GetCursorTextIndex(textX, &substring);
//			if (IsPassword()) //! @ todo
//			{
//				pos = ConvertFromPasswordPosition(pos);
//				if (TTF_GetTextSubString(_pText, pos, &substring))
//					pos = GetCursorTextIndex(textX, &substring);
//			}
			if (IsShiftDown())
			{
				auto last_cursor = _body.GetCursor();
				SetCursorPosition(pos);
				OnMoveCursor(last_cursor.position);
			}
			else
			{
				SetCursorPosition(pos);
				_bIsHighlighting = true;
				highlight_start = _body.GetCursorPosition();
				highlight_end = -1;
			}
		}

		return true;
	}

	bool TextInput2::HandleMouseMotion(int x, int y)
	{
		bool bHandled = false;
		auto rect = GetClientRect();

		if (_bIsHighlighting)
		{
			/* Set the highlight position */
			TTF_SubString substring;
			int textX = x - rect.x + _scroll.x;
			int textY = y - rect.y + _scroll.y;
			if (TTF_GetTextSubStringForPoint(GetRenderedText(), textX, textY, &substring))
			{
				int32_t pos = GetCursorTextIndex(textX, &substring);
//				if (IsPassword()) //! @todo
//				{
//					pos = ConvertFromPasswordPosition(pos);
//					if (TTF_GetTextSubString(_pText, pos, &substring))
//						pos = GetCursorTextIndex(textX, &substring);
//				}

				SetCursorPosition(pos);
				highlight_end = _body.GetCursorPosition();

				bHandled = true;
			}
		}


		// Change cursor
		fig::point pt = { x, y };
		bool bInRect = SDL_PointInRect(&pt, &rect);
		if (bInRect != _bIBeamCursor)
		{
			_bIBeamCursor = bInRect;

			if (_bIBeamCursor)
				PushEvent(UserEvent::PushCursor, Cursor::Caret);
			else
				PushEvent(UserEvent::PopCursor, Cursor::Caret);
			//Global::SetCursor(_bIBeamCursor ? SDL_SYSTEM_CURSOR_TEXT : SDL_SYSTEM_CURSOR_DEFAULT);
			bHandled = true;
		}

		return bHandled;
	}

	bool TextInput2::HandleMouseUp(int x, int y)
	{
		if (!_bIsHighlighting)
			return false;

		_bIsHighlighting = false;
		return true;
	}

	void TextInput2::SelectAll()
	{
		_body.SelectAll();
	}

	void TextInput2::Deselect()
	{
		_body.Deselect();
	}

#pragma endregion Selection

	void TextInput2::Copy()
	{
		_body.Copy();
	}

	void TextInput2::Cut()
	{
		if (_body.Cut())
		{
			PushUndo(UndoAction::Erase, false);
			DidChange();
		}
	}

	void TextInput2::Paste()
	{
		if (!IsMultiline())
		{
			// Only accept the first line of pasted content
			wstring content = from_utf8(SDL_GetClipboardText());
			normalize_newlines(content);
			size_t pos_endl = index_of(content, 0, L'\n');
			if (pos_endl != fig::npos)
				content.resize(pos_endl);

			string contentUtf8 = to_utf8(content);
			Insert(contentUtf8);
		}
		else
		{
			Insert(SDL_GetClipboardText());
		}

		PushUndo(UndoAction::Write, false);
		DidChange();
	}

	void TextInput2::Insert(fig::string_view text)
	{
		if (text.empty())
			return;

		DeleteHighlight();

//		if (composition_length > 0) //! @todo
//		{
//			TTF_DeleteTextString(_pText, composition_start, composition_length);
//			composition_length = 0;
//		}

		_body.Insert(text);
	}

#pragma region Events

	EventResult TextInput2::OnEvent(fig::event& event)
	{
		bool bCtrl = event.key.mod & SDL_KMOD_CTRL;
		bool bShift = event.key.mod & SDL_KMOD_SHIFT;
		bool bAlt = event.key.mod & SDL_KMOD_ALT;

		bool bModCtrl = bCtrl and not (bShift or bAlt);
		bool bModShift = bShift and not (bCtrl or bAlt);
		bool bModAlt = bAlt and not (bCtrl or bShift);
		bool bModCtrlShift = bCtrl and bShift and not bAlt;
		bool bModNone = not (bCtrl or bShift or bAlt);

		if (not GetEnabled())
			return EventResult::Pass; // Disabled

		switch (event.type)
		{
		case SDL_EVENT_MOUSE_MOTION:
			return HandleMouseMotion(toI(event.motion.x), toI(event.motion.y)) ? EventResult::Handled : EventResult::Pass;
		case SDL_EVENT_MOUSE_BUTTON_DOWN:
			return HandleMouseDown(toI(event.button.x), toI(event.button.y)) ? EventResult::Handled : EventResult::Pass;
		case SDL_EVENT_MOUSE_BUTTON_UP:
			HandleMouseUp(toI(event.button.x), toI(event.button.y)) ? EventResult::Handled : EventResult::Pass;
			break;
		}

		if (!_bFocused)
			return EventResult::Pass;

		switch (event.type)
		{
		case SDL_EVENT_KEY_DOWN:
		{
#if _DEBUG
			if (bModAlt)
			{
				switch (event.key.key)
				{
				case SDLK_UP:
					_scroll.y -= 10;
					return EventResult::Handled;
				case SDLK_DOWN:
					_scroll.y += 10;
					return EventResult::Handled;
				case SDLK_LEFT:
					_scroll.x -= 10;
					return EventResult::Handled;
				case SDLK_RIGHT:
					_scroll.x += 10;
					return EventResult::Handled;
				}
			}
#endif
			switch (event.key.key)
			{
			case SDLK_A:
				if (bModCtrl)
				{
					SelectAll();
					return EventResult::Handled;
				}
				break;
			case SDLK_C:
				if (bModCtrl)
				{
					Copy();
					return EventResult::Handled;
				}
				break;

			case SDLK_V:
				if (bModCtrl)
				{
					Paste();
					return EventResult::Handled;
				}
				break;

			case SDLK_X:
				if (bModCtrl)
				{
					Cut();
					return EventResult::Handled;
				}
				break;

			case SDLK_Z:
				if (bModCtrl)
				{
					Undo();
					return EventResult::Handled;
				}
				break;

			case SDLK_Y:
				if (bModCtrl)
				{
					Redo();
					return EventResult::Handled;
				}
				break;

			case SDLK_LEFT:
				if (bModCtrl or bModCtrlShift)
				{
					MoveCursorToPriorWord();
					return EventResult::Handled;
				}
				else if (bModNone or bModShift)
				{
					MoveCursorLeft();
					return EventResult::Handled;
				}
				break;

			case SDLK_RIGHT:
				if (bModCtrl or bModCtrlShift)
				{
					MoveCursorToNextWord();
					return EventResult::Handled;
				}
				else if (bModNone or bModShift)
				{
					MoveCursorRight();
					return EventResult::Handled;
				}
				break;

			case SDLK_UP:
				if (bModNone or bModShift)
				{
					MoveCursorUp();
					return EventResult::Handled;
				}
				break;

			case SDLK_DOWN:
				if (bModNone or bModShift)
				{
					MoveCursorDown();
					return EventResult::Handled;
				}
				break;

			case SDLK_HOME:
				if (bModCtrl or bModCtrlShift)
				{
					MoveCursorBeginning();
					return EventResult::Handled;
				}
				else if (bModNone or bModShift)
				{
					MoveCursorBeginningOfLine();
					return EventResult::Handled;
				}
				break;

			case SDLK_END:
				if (bModCtrl or bModCtrlShift)
				{
					MoveCursorEnd();
					return EventResult::Handled;
				}
				else if (bModNone or bModShift)
				{
					MoveCursorEndOfLine();
					return EventResult::Handled;
				}
				break;

			case SDLK_BACKSPACE:
				if (bModCtrlShift)
				{
					BackspaceToBeginningOfLine();
					return EventResult::Handled;
				}
				else
					if (bModCtrl)
					{
						BackspaceToPriorWord();
						return EventResult::Handled;
					}
					else if (bModNone || bModShift)
					{
						Backspace();
						return EventResult::Handled;
					}
				break;

			case SDLK_DELETE:
				if (bModCtrlShift)
				{
					DeleteToEndOfLine();
					return EventResult::Handled;
				}
				else if (bModCtrl)
				{
					DeleteToNextWord();
					return EventResult::Handled;
				}
				else if (bModNone || bModShift)
				{
					Delete();
					return EventResult::Handled;
				}
				break;

			case SDLK_RETURN:
			case SDLK_KP_ENTER:
				if (IsMultiline() and (not _flags.IsSet(Flag::CtrlEnterNewLine) or (bModCtrl || bModShift)))
				{
					Insert("\n");
					PushUndo(UndoAction::Write, false);
					DidChange();
					return EventResult::Handled;
				}
				else if (bModNone and _pOnEnter)
				{
					_pOnEnter(_body.GetText()); // Invoke
					return EventResult::Handled;
				}
				break;

			case SDLK_ESCAPE:
				if (bModNone)
				{
					if (HasSelection())
					{
						Deselect();
						return EventResult::Handled;
					}
					else
					{
						SetFocus(false);
						return EventResult::Handled;
					}
				}
				break;

			default:
				return EventResult::Pass;
			}
			break;
		}
		case SDL_EVENT_TEXT_INPUT:
			Insert(event.text.text);
			if (event.text.text)
			{
				if (is_whitespace(event.text.text[0]) || is_punctuation(event.text.text[0]))
					PushUndo(UndoAction::WhitespacePunctuation);
				else
					PushUndo(UndoAction::Write);
				DidChange();
			}
			return EventResult::Handled;

		case SDL_EVENT_TEXT_EDITING:
			HandleComposition(&event.edit);
			break;

		case SDL_EVENT_TEXT_EDITING_CANDIDATES:
			ClearCandidates();
			SaveCandidates(&event);
			break;

		default:
			break;
		}

		if (_bFocused and IsUserEvent(event, UserEvent::ScreenDeactivated))
		{
			SetFocus(false);
			return EventResult::Continue;
		}
		else if (_bFocused and IsUserEvent(event, UserEvent::StartTextInput) and event.user.data1 != this)
		{
			SetFocus(false);
			return EventResult::Continue;
		}

		return EventResult::Pass;
	}

#pragma endregion Events
	void TextInput2::OnSize()
	{
		if (IsMultiline())
		{
			int width = std::max(GetWidth() - GetMarginHorizontal(), 0);
			int currWrapWidth;
			if (TTF_GetTextWrapWidth(GetRenderedText(), &currWrapWidth) && currWrapWidth != width)
				TTF_SetTextWrapWidth(GetRenderedText(), width);

			_body.SetTextWrapWidth(width);
		}
		else
		{
			TTF_SetTextWrapWidth(GetRenderedText(), 0);
		}
	}

	void TextInput2::SetTextChangedCallback(TextChangedCallback cb)
	{
		_pOnChanged = cb;
	}

	void TextInput2::SetEnterPressedCallback(EnterPressedCallback cb)
	{
		_pOnEnter = cb;
	}

	void TextInput2::Clear()
	{
		_body.Clear();
		InitUndo();
		DidChange();
		_scroll = {};
	}

	void TextInput2::SetText(fig::string_view text)
	{
		Clear();
		Insert(text);
		InitUndo();
		DidChange();
		_scroll = {};
	}

	void TextInput2::SetPlaceholder(fig::string_view text)
	{
		TTF_SetTextString(_pPlaceholder, text.data(), text.length());
	}

	fig::string TextInput2::GetText() const
	{
		return _body.GetText();
	}

	void TextInput2::ApplyScroll(int& x, int& y) const
	{
		x -= toI(_scroll.x);
		y -= toI(_scroll.y);
	}

	void TextInput2::ApplyScroll(float& x, float& y) const
	{
		x -= _scroll.x;
		y -= _scroll.y;
	}

	void TextInput2::ApplyScroll(fig::rect& rect) const
	{
		rect.x -= _scroll.x;
		rect.y -= _scroll.y;
	}

	void TextInput2::ApplyScroll(fig::rectf& rect) const
	{
		rect.x -= _scroll.x;
		rect.y -= _scroll.y;
	}

	void TextInput2::SetMinRows(int32_t rows)
	{
		_minRows = std::max(rows, 1);
		_maxRows = std::max(_minRows, _maxRows);
	}

	void TextInput2::SetMaxRows(int32_t rows)
	{
		_maxRows = std::max(rows, 1);
		_minRows = std::min(_minRows, _maxRows);
	}

	void TextInput2::Autosize()
	{
		if (!IsAutosized())
			return;

		auto clientRect = GetClientRect();
		int lineSkip = TTF_GetFontLineSkip(_pFont);
		int numRows = _body.GetLineCount();

		numRows = std::clamp(numRows, _minRows, _maxRows);
		if (numRows * lineSkip != clientRect.h)
		{
			auto& rect = GetRect();
			SetHeight(numRows * lineSkip + GetMarginVertical());
			InvalidateParentLayout(false);
		}
	}

	void TextInput2::UpdatePassword()
	{
		if (!_pPassword)
			return;

		auto& text = _body.GetText();
		size_t length = SDL_utf8strlen(text.c_str());
		if (_pPassword->text && _lastLength == length)
			return;

		_lastLength = length;

		wstring wdots;
		wdots.resize(length);
		for (size_t i = 0; i < length; ++i)
			wdots[i] = L'\u2022'; // Heavy asterisk
		string dots = to_utf8(wdots);

		TTF_SetTextString(_pPassword, toCStr(dots), 0);
	}

	int32_t TextInput2::ConvertToPasswordPosition(int32_t position)
	{
		if (IsPassword())
		{
			int utf8_position = BytesUTF8Length(_body.GetText().c_str(), position);
			return UTF8ByteLength(_pPassword->text, utf8_position);
		}
		return position;
	}

	int32_t TextInput2::ConvertFromPasswordPosition(int32_t position)
	{
		if (IsPassword())
		{
			int utf8_position = BytesUTF8Length(_pPassword->text, position);
			return UTF8ByteLength(_body.GetText().c_str(), utf8_position);
		}
		return position;
	}

#pragma region Undo/Redo

	TextInput2::UndoState TextInput2::GetUndoState(UndoAction action) const noexcept
	{
		return UndoState
		{
			.text = GetText(),
			.cursor_pos = _body.GetCursorPosition(),
			.highlight_start = highlight_start,
			.highlight_end = highlight_end,
			.actionType = action,
		};
	}

	void TextInput2::InitUndo() noexcept
	{
		_undo.SetInitState(GetUndoState(UndoAction::Default));
	}

	void TextInput2::PushUndo(UndoAction action, bool allowCoalesce)
	{
		_undo.PushState(GetUndoState(action));
		_undo.CreateUndo(allowCoalesce);
	}

	void TextInput2::Undo()
	{
		if (auto undo = _undo.Undo())
		{
//			TTF_SetTextString(_pText, toCStr(undo.value().text), 0); //! @todo
			SetCursorPosition(undo.value().cursor_pos);
			highlight_start = undo.value().highlight_start;
			highlight_end = undo.value().highlight_end;
		}
	}

	void TextInput2::Redo()
	{
		if (auto undo = _undo.Redo())
		{
//			TTF_SetTextString(_pText, toCStr(undo.value().text), 0); //! @todo
			SetCursorPosition(undo.value().cursor_pos);
			highlight_start = undo.value().highlight_start;
			highlight_end = undo.value().highlight_end;
		}
	}

#pragma endregion Undo/Redo

	void TextInput2::OnPostRender()
	{
		Autosize();
	}

	void TextInput2::DidChange()
	{
		if (_pOnChanged)
			_pOnChanged(_body.GetText());

		OnText(_body.GetText());
	}

	void TextInput2::OnEnabled(bool bEnabled)
	{
		if (not bEnabled)
		{
			SetFocus(false);
			Deselect();
			SetForegroundColor(Color::DisabledForeground);
		}
		else
		{
			SetForegroundColor(Color::Black);
		}
	}
}