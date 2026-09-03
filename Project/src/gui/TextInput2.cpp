#include <pch.h>
#include "gui/TextInput2.h"
#include "gui/Window.h"
#include "app/AppState.h"
#include <algorithm>

constexpr uint64_t CursorBlinkIntervalMS { 500ULL };

static int Utf8CodepointsToBytes(const char* text, int num_codepoints)
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

static int GetUtf8Codepoints(const char* text, int position)
{
	if (!text)
		return 0;

	const char* start = text;
	const char* end = text + position;
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

static bool StepLeft(fig::string_view text, int32_t& position)
{
	if (position <= 0)
		return false;

	assert(position <= static_cast<int32_t>(text.size()));

	const char* pos = text.data() + position;
	if (SDL_StepBackUTF8(text.data(), &pos) != SDL_INVALID_UNICODE_CODEPOINT)
	{
		position = (int32_t)(std::distance(text.data(), pos));
		return true;
	}
	return false; // Error
}

static bool StepRight(fig::string_view text, int32_t& position)
{
	if (position >= static_cast<int32_t>(text.size()))
		return false;

	const char* pos = text.data() + position;
	while (SDL_StepUTF8(&pos, NULL) == SDL_INVALID_UNICODE_CODEPOINT);
	position = (int32_t)(std::distance(text.data(), pos));
	return true;
}

enum CharType 
{
	Whitespace,
	Punctuation,
	Letter,
};

static CharType GetCharType(char ch) 
{
	if (fig::is_whitespace(ch)) return CharType::Whitespace;
	if (fig::is_punctuation(ch)) return CharType::Punctuation;
	return CharType::Letter;
};

static int32_t FindPriorWord(fig::string_view text, int32_t position)
{
	if (position <= 0)
		return 0;

	// Skip any whitespace first
	int32_t curr = position;
	while (curr > 0)
	{
		if (not StepLeft(text, curr))
			break;

		CharType charType = GetCharType(text[curr]);
		if (charType != CharType::Whitespace)
		{
			StepRight(text, curr);
			break;
		}
	}

	// Find first position of word
	if (StepLeft(text, curr))
	{
		CharType skipType = GetCharType(text[curr]);
		while (curr > 0)
		{
			if (not StepLeft(text, curr))
				break;
			CharType priorType = GetCharType(text[curr]);
			if (priorType == skipType)
				continue;

			StepRight(text, curr);
			break;
		}
	}

	return curr;
}

static int32_t FindNextWord(fig::string_view text, int32_t position)
{
	int32_t curr = position;
	int32_t length = static_cast<int32_t>(text.size());
	if (position >= length)
		return length;

	CharType startType = GetCharType(text[position]);
	if (startType == CharType::Whitespace)
	{
		// Skip to first non-whitespace
		while (fig::is_whitespace(text[curr]))
		{
			if (not StepRight(text, curr))
				break;
		}
		return curr;
	}
	else
	{
		// Skip similar
		CharType nextType = startType;
		while (nextType == startType)
		{
			if (not StepRight(text, curr) or curr == length)
				break;
			nextType = GetCharType(text[curr]);
		}

		// Skip whitespace
		while (curr < length - 1 and nextType == CharType::Whitespace)
		{
			if (not StepRight(text, curr) or curr == length)
				break;
			nextType = GetCharType(text[curr]);
		}
		return curr;
	}
}

static int GetCursorTextIndex(int32_t x, const TTF_SubString* substring)
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

		if (flags.IsSet(Flag::Password))
			_pPassword = TTF_CreateText(GetSDLTextEngine(), _pFont, nullptr, 0);

		_bFocused = false;
		EnableClipping(false);

		if (_pFont)
		{
			_lineHeight = TTF_GetFontLineSkip(_pFont);
			SetSize(300, MeasureFontHeight(*_pFont) + GetMarginVertical());
		}
	}

	TextInput2::~TextInput2()
	{
		ClearCandidates();
		TTF_DestroyText(_pPassword);
		TTF_DestroyText(_pPlaceholder);
	}

	void TextInput2::SetTextWrapWidth(int32_t width)
	{
		if (_wrapWidth == width)
			return;

		_wrapWidth = std::max(width, 0);
		RelayoutAll();
	}

	void TextInput2::SetFont(FontFace fontFace, double ptSize) noexcept
	{
		if (auto font = Fonts::GetFont(fontFace, ptSize))
		{
			_pFont = font;
			_lineHeight = TTF_GetFontLineSkip(_pFont);

			RelayoutAll();
		}
	}

	int32_t TextInput2::GetTextWrapWidth() const noexcept
	{
		return _wrapWidth;
	}

	size_t TextInput2::GetLineCount() const noexcept
	{
		return _lines.size();
	}

	void TextInput2::OnUpdate(float fElapsed)
	{
		uint64_t now = SDL_GetTicks();
		if ((now - _last_cursor_change) >= CursorBlinkIntervalMS)
		{
			_cursor_visible = !_cursor_visible;
			_last_cursor_change = now;
		}

		if (_bFocused)
		{
			auto& rect = GetRect();
			_cursor_rect = GetCursorRect();
			_cursor_rect.x += rect.x + GetMarginLeft();
			_cursor_rect.y += rect.y + GetMarginTop();
			UpdateTextInputArea();
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
			if (IsMultiline()) // Vertical scroll
			{
				float cursorY = _cursor_rect.y - clientRect.y;
				while (toI(std::round((cursorY - _scroll.y) / lineSkip)) >= maxRows)
					_scroll.y += lineSkip;
				while (toI(std::round((cursorY - _scroll.y) / lineSkip)) < 0)
					_scroll.y -= lineSkip;
			}
			else
			{
				_scroll.y = 0;
			}

			if (not IsWordWrapping()) // Horizontal scroll
			{
				constexpr int32_t kScrollStep = 80;

				if (auto pText = IsPassword() ? _pPassword.get() : (_lines.empty() ? nullptr : _lines[0].ttf_text.get()))
				{
					int maxCursorX = clientRect.w;
					int textWidth, _;
					TTF_GetTextSize(pText, &textWidth, &_);
					int cursorX = toI(_cursor_rect.x + _cursor_rect.w - clientRect.x);
					while (cursorX > 0 and cursorX - _scroll.x > maxCursorX)
						_scroll.x = std::min(_scroll.x + kScrollStep, cursorX - maxCursorX);
					while (cursorX > 0 and cursorX - _scroll.x < 0)
						_scroll.x = std::max(_scroll.x - kScrollStep, 0);
				}
			}
			else
			{
				_scroll.x = 0;
			}
		}

		// Draw highlight(s) 
		if (HasSelection())
		{
			if (auto highlights = GetHighlights(); not highlights.empty())
			{
				SDL_SetRenderDrawColor(pRenderer, Color::TextSelectionBackground.r, Color::TextSelectionBackground.g, Color::TextSelectionBackground.b, Color::TextSelectionBackground.a);
				for (auto& highlight_rect : highlights)
				{
					highlight_rect.w = std::max(highlight_rect.w, 3.0f);
					highlight_rect.x += rect.x + GetMarginLeft();
					highlight_rect.y += rect.y + GetMarginTop();

					ApplyScroll(highlight_rect);
					SDL_RenderFillRect(pRenderer, &highlight_rect);
				}
			}
		}

		if (not _text.empty())
		{
			if (IsPassword())
				DrawText(pRenderer, _pPassword, 0, 0);
			else
			{
				for (size_t i = 0uz; i < _lines.size(); ++i)
				{
					auto& line = _lines[i];

					if (_composition_cursor_length > 0 and _composition_line == i and not _composition_text.empty())
						DrawText(pRenderer, _composition_text.get(), 0, _lineHeight * static_cast<int32_t>(i));
					else if (line.ttf_text->text)
						DrawText(pRenderer, line.ttf_text.get(), 0, _lineHeight * static_cast<int32_t>(i));
				}
			}
		}
		else
			DrawPlaceholder(pRenderer, rect.x, rect.y + 8);

		if (_bFocused)
		{
			if (_composition_length > 0)
				DrawComposition(pRenderer);

			if (_candidates)
				DrawCandidates(pRenderer);

			if (_cursor_visible)
				DrawCursor(pRenderer);
		}

		SDL_SetRenderClipRect(pRenderer, restoreClipping ? &prevClippingRect : nullptr);
	}

	void TextInput2::DrawText(fig::renderer_ptr pRenderer, TTF_Text* pText, int x, int y)
	{
		auto fgColor = GetForegroundColor();
		TTF_SetTextColor(pText, fgColor.r, fgColor.g, fgColor.b, fgColor.a);

		auto& rect = GetRect();
		int xx = rect.x + GetMarginLeft() + x;
		int yy = rect.y + GetMarginTop() + y;
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
		if (_composition_length > 0)
		{
			DrawCompositionCursor(pRenderer);
			return;
		}

		auto rect = _cursor_rect;
		ApplyScroll(rect);
		SDL_SetRenderDrawColor(pRenderer, 0, 0, 0, 0xFF);
		SDL_RenderFillRect(pRenderer, &rect);
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
		_composition_start = 0;
		_composition_length = 0;
		_composition_cursor = 0;
		_composition_cursor_length = 0;
		_composition_text.release();
		_composition_line = -1;
	}

	void TextInput2::HandleComposition(const SDL_TextEditingEvent* event)
	{
		DeleteSelection();

		auto cursor = GetCursor();
		_composition_line = cursor.line;
		auto& line = _lines[_composition_line];
		if (line.ttf_text.empty())
			return;

		int length = (int)SDL_strlen(event->text);
		LogLn(std::format("Composition: {}", length));
		if (length > 0)
		{
			if (_composition_text.empty())
				_composition_text = fig::sdl::Text(GetSDLTextEngine(), _pFont, _text.data(), line.length);
			if (_composition_length > 0)
				TTF_DeleteTextString(_composition_text.get(), _composition_start, _composition_length);

			_composition_start = cursor.offset;
			_composition_length = length;
			TTF_InsertTextString(_composition_text.get(), _composition_start, event->text, _composition_length);
			if (event->start > 0 or event->length > 0)
			{
				_composition_cursor = Utf8CodepointsToBytes(&_composition_text->text[_composition_start], event->start);
				_composition_cursor_length = Utf8CodepointsToBytes(&_composition_text->text[_composition_start + _composition_cursor], event->length);
			}
			else
			{
				_composition_cursor = length;
				_composition_cursor_length = 0;
			}
		}
		else
		{
			ResetComposition();
		}
	}

	void TextInput2::CancelComposition()
	{
		ResetComposition();
		SDL_ClearComposition(GetSDLWindow());
	}

	void TextInput2::DrawComposition(fig::renderer_ptr pRenderer)
	{
		auto clientRect = GetClientRect();
		auto fgColor = GetForegroundColor();

		/* Draw an underline under the composed text */
		int font_height = TTF_GetFontHeight(_pFont);
		TTF_SubString** substrings = TTF_GetTextSubStringsForRange(_composition_text.get(), _composition_start, _composition_length, NULL);
		if (substrings)
		{
			for (int i = 0; substrings[i]; ++i)
			{
				fig::rectf line_rect;
				SDL_RectToFRect(&substrings[i]->rect, &line_rect);
				line_rect.x += clientRect.x;
				line_rect.y += clientRect.y + _composition_line * _lineHeight + font_height;
				line_rect.h = 1.0f;
				ApplyScroll(line_rect);
				SDL_SetRenderDrawColor(pRenderer, fgColor.r, fgColor.g, fgColor.b, 0xFF);
				SDL_RenderFillRect(pRenderer, &line_rect);
			}
			SDL_free(substrings);
		}

		/* Thicken the underline under the active clause in the composed text */
		if (_composition_cursor_length > 0)
		{
			substrings = TTF_GetTextSubStringsForRange(_composition_text.get(), _composition_start + _composition_cursor, _composition_cursor_length, NULL);
			if (substrings)
			{
				for (int i = 0; substrings[i]; ++i)
				{
					fig::rectf line_rect;
					SDL_RectToFRect(&substrings[i]->rect, &line_rect);
					line_rect.x += clientRect.x;
					line_rect.y += clientRect.y + _composition_line * _lineHeight + font_height;
					line_rect.h = 2.0f;
					ApplyScroll(line_rect);

					SDL_SetRenderDrawColor(pRenderer, fgColor.r, fgColor.g, fgColor.b, 0xFF);
					SDL_RenderFillRect(pRenderer, &line_rect);
				}
				SDL_free(substrings);
			}
		}
	}

	void TextInput2::DrawCompositionCursor(fig::renderer_ptr pRenderer)
	{
		auto clientRect = GetClientRect();

		if (_composition_cursor_length == 0)
		{
			TTF_SubString cursor;
			if (TTF_GetTextSubString(_composition_text.get(), _composition_start + _composition_cursor, &cursor))
			{
				fig::rectf cursor_rect = to_rectf(cursor.rect);
				cursor_rect.x += clientRect.x;
				cursor_rect.y += clientRect.y + _composition_line * _lineHeight;
				cursor_rect.w = 1.0f;

				ApplyScroll(cursor_rect);

				SDL_SetRenderDrawColor(pRenderer, 0, 0, 0, 0xFF);
				SDL_RenderFillRect(pRenderer, &cursor_rect);
			}
		}
	}

	void TextInput2::ClearCandidates()
	{
		if (_candidates)
			_candidates.release();
		_selected_candidate_start = 0;
		_selected_candidate_length = 0;
	}

	void TextInput2::SaveCandidates(const SDL_Event* event)
	{
		ClearCandidates();

		bool horizontal = event->edit_candidates.horizontal;
		int num_candidates = event->edit_candidates.num_candidates;
		int selected_candidate = event->edit_candidates.selected_candidate;

		/* Calculate the length of the candidates text */
		size_t length = 0;
		for (int i = 0; i < num_candidates; ++i)
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
		for (int i = 0; i < num_candidates; ++i)
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
				_selected_candidate_start = (int)(uintptr_t)(dst - candidate_text);
				_selected_candidate_length = length;
			}
			SDL_memcpy(dst, event->edit_candidates.candidates[i], length);
			dst += length;

			if (!horizontal)
			{
				*dst++ = '\n';
			}
		}
		*dst = '\0';

		_candidates = fig::sdl::Text(GetSDLTextEngine(), _pFont, candidate_text, 0);
		SDL_free(candidate_text);
		if (_candidates)
		{
			float r, g, b, a;
			TTF_GetTextColorFloat(_composition_text.get(), &r, &g, &b, &a);
			TTF_SetTextColorFloat(_candidates.get(), r, g, b, a);
		}
		else
		{
			ClearCandidates();
		}
	}

	void TextInput2::DrawCandidates(fig::renderer_ptr pRenderer)
	{
		SDL_Rect safe_rect;
		fig::rectf candidates_rect;
		int candidates_w;
		int candidates_h;

		auto rect = GetClientRect();

		/* Position the candidate window */
		TTF_SubString cursor;
		int offset = _composition_start;
		if (_composition_cursor_length > 0)
		{
			// Place the candidates at the active clause
			offset += _composition_cursor;
		}
		if (!TTF_GetTextSubString(_composition_text.get(), offset, &cursor))
			return;

		SDL_GetRenderSafeArea(pRenderer, &safe_rect);
		TTF_GetTextSize(_candidates.get(), &candidates_w, &candidates_h);
		candidates_rect.x = toF(rect.x + cursor.rect.x);
		candidates_rect.y = toF(rect.y + cursor.rect.y + cursor.rect.h + 2);
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

//		ApplyScroll(candidates_rect);

		/* Draw the candidate background */
		SDL_SetRenderDrawColor(pRenderer, 0xAA, 0xAA, 0xAA, 0xFF);
		SDL_RenderFillRect(pRenderer, &candidates_rect);
		SDL_SetRenderDrawColor(pRenderer, 0x00, 0x00, 0x00, 0xFF);
		SDL_RenderRect(pRenderer, &candidates_rect);

		/* Draw the candidates */
		int x = toI(candidates_rect.x + 3);
		int y = toI(candidates_rect.y + 3);
		DrawText(pRenderer, _candidates.get(), x, y);

		/* Underline the selected candidate */
		if (_selected_candidate_length > 0)
		{
			int font_height = TTF_GetFontHeight(_pFont);
			TTF_SubString** substrings = TTF_GetTextSubStringsForRange(_candidates.get(), _selected_candidate_start, _selected_candidate_length, NULL);
			if (substrings)
			{
				for (int i = 0; substrings[i]; ++i)
				{
					fig::rectf rect;
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
			PushEvent(UserEvent::StartTextInput, 0, this);
		else
			PushEvent(UserEvent::StopTextInput, 0, this);
	}

	int32_t TextInput2::MoveCursor(int32_t direction) noexcept
	{
		auto last_cursor = _cursor;

		auto position = _cursor;
		if (direction < 0 and not StepLeft(_text, position))
			return _cursor;
		if (direction > 0 and not StepRight(_text, position))
			return _cursor;
		
		SetCursor(position);
		OnMoveCursor(last_cursor);
		return _cursor;
	}

	void TextInput2::OnMoveCursor(int32_t last_position)
	{
		bool isShiftDown = IsShiftDown();
		bool is_highlighting = HasSelection();
		if (!is_highlighting and isShiftDown)
		{
			Select(last_position, last_position);
			is_highlighting = true;
		}
		else if (is_highlighting and !isShiftDown)
		{
			Deselect();
			is_highlighting = false;
		}

		if (is_highlighting)
		{
			auto [start, end] = GetSelection();
			if (start == last_position)
				start = _cursor;
			else
				end = _cursor;
			Select(start, end);
		}

		ResetCursorBlink();
	}

	int32_t TextInput2::MoveCursorLeft() noexcept
	{
		return MoveCursor(-1);
	}

	int32_t TextInput2::MoveCursorRight() noexcept
	{
		return MoveCursor(1);
	}

	int32_t TextInput2::MoveCursorUp() noexcept
	{
		if (not IsMultiline())
			return _cursor;

		if (_lines.empty())
			return 0;

		if (IsOnLastNewLine())
		{
			auto last_cursor = _cursor;
			SetCursor(_lines.back().position);
			OnMoveCursor(last_cursor);
			return _cursor;
		}

		auto cursor = GetCursor();
		if (cursor.line == 0)
			return _cursor;


		auto& curr_line = _lines[cursor.line];
		TTF_SubString substring;
		if (TTF_GetTextSubString(curr_line.ttf_text.get(), cursor.offset, &substring))
		{
			int32_t x = substring.rect.x;
			auto& prev_line = _lines[cursor.line - 1];
			if (TTF_GetTextSubStringForPoint(prev_line.ttf_text.get(), x, _lineHeight / 2, &substring))
			{
				auto last_cursor = _cursor;
				SetCursor(prev_line.position + GetCursorTextIndex(x, &substring));
				OnMoveCursor(last_cursor);
			}
		}
		return _cursor;
	}

	int32_t TextInput2::MoveCursorDown() noexcept
	{
		if (not IsMultiline())
			return _cursor;

		if (_lines.empty())
			return 0;

		auto cursor = GetCursor();
		if (cursor.line + 1uz >= _lines.size())
			return SetCursor(static_cast<int32_t>(_text.size()));

		auto& curr_line = _lines[cursor.line];
		auto& next_line = _lines[cursor.line + 1];
		TTF_SubString substring;
		int32_t pos = next_line.position;
		if (TTF_GetTextSubString(curr_line.ttf_text.get(), cursor.offset, &substring))
		{
			int32_t x = substring.rect.x;
			if (TTF_GetTextSubStringForPoint(next_line.ttf_text.get(), x, _lineHeight / 2, &substring))
				pos += GetCursorTextIndex(x, &substring);
		}

		auto last_cursor = _cursor;
		SetCursor(pos);
		OnMoveCursor(last_cursor);
		return _cursor;
	}

	int32_t TextInput2::MoveCursorBeginningOfLine() noexcept
	{
		if (_lines.empty())
			return 0;

		if (IsOnLastNewLine())
			return _cursor;

		auto last_cursor = _cursor;
		auto& line = _lines[GetCursor().line];
		SetCursor(line.position);
		OnMoveCursor(last_cursor);
		return _cursor;
	}

	int32_t TextInput2::MoveCursorEndOfLine() noexcept
	{
		if (_lines.empty())
			return _cursor;

		if (_cursor == _text.size())
			return _cursor;

		auto last_cursor = _cursor;
		auto& line = _lines[GetCursor().line];
		SetCursor(line.position + line.length - (IsEOL(line) ? 1 : 0));
		OnMoveCursor(last_cursor);
		return _cursor;
	}

	int32_t TextInput2::MoveCursorToPriorWord() noexcept
	{
		auto last_cursor = _cursor;
		SetCursor(FindPriorWord(_text, _cursor));
		OnMoveCursor(last_cursor);
		return _cursor;
	}

	int32_t TextInput2::MoveCursorToNextWord() noexcept
	{
		if (IsPassword())
			return _cursor;

		auto last_cursor = _cursor;
		SetCursor(FindNextWord(_text, _cursor));
		OnMoveCursor(last_cursor);
		return _cursor;
	}

	int32_t TextInput2::MoveCursorBeginning() noexcept
	{
		auto last_cursor = _cursor;
		SetCursor(0);
		OnMoveCursor(last_cursor);
		return _cursor;
	}

	int32_t TextInput2::MoveCursorEnd() noexcept
	{
		auto last_cursor = _cursor;
		SetCursor(static_cast<int32_t>(_text.size()));
		OnMoveCursor(last_cursor);
		return _cursor;
	}

	bool TextInput2::Backspace()
	{
		if (DeleteSelection())
			return true;

		if (_cursor > 0)
		{
			int32_t pos = _cursor;
			if (StepLeft(_text, pos))
			{
				if (Delete(pos, _cursor - pos))
				{
					ResetCursorBlink();
					PushUndo(UndoAction::Erase);
					DidChange();
					return true;
				}
			}
		}
		return false;
	}

	bool TextInput2::BackspaceToPriorWord()
	{
		if (DeleteSelection())
			return true;

		int32_t prior = FindPriorWord(_text, _cursor);
		int32_t length = (_cursor - prior);
		if (Delete(prior, length))
		{
			ResetCursorBlink();
			PushUndo(UndoAction::Erase, false);
			DidChange();
			return true;
		}
		return false;
	}

	bool TextInput2::BackspaceToBeginning()
	{
		if (DeleteSelection())
			return true;

		if (Delete(0, _cursor))
		{
			ResetCursorBlink();
			PushUndo(UndoAction::Erase, false);
			DidChange();
			return true;
		}
		return false;
	}

	bool TextInput2::BackspaceToBeginningOfLine()
	{
		if (DeleteSelection())
			return true;

		auto cursor = GetCursor();
		if (Delete(cursor.position, _cursor))
		{
			ResetCursorBlink();
			PushUndo(UndoAction::Erase, false);
			DidChange();
			return true;
		}
		return false;
	}

	bool TextInput2::Delete()
	{
		if (DeleteSelection())
			return true;

		const char* start = &_text[_cursor];
		const char* next = start;
		size_t length = SDL_strlen(next);
		SDL_StepUTF8(&next, &length);
		length = static_cast<size_t>((uintptr_t)next - (uintptr_t)start);

		if (Delete(_cursor, static_cast<int32_t>(length)))
		{
			ResetCursorBlink();
			PushUndo(UndoAction::Erase);
			DidChange();
			return true;
		}
		return false;
	}

	bool TextInput2::DeleteToNextWord()
	{
		if (DeleteSelection())
			return true;

		int32_t next = FindNextWord(_text, _cursor);
		int32_t length = next - _cursor;
		if (Delete(_cursor, length))
		{
			ResetCursorBlink();
			PushUndo(UndoAction::Erase, false);
			DidChange();
			return true;
		}
		return false;
	}

	bool TextInput2::DeleteToEndOfLine()
	{
		if (DeleteSelection())
			return true;

		auto cursor = GetCursor();
		if (cursor.line < _lines.size() and Delete(_cursor, _lines[cursor.line].position + _lines[cursor.line].length - cursor.position))
		{
			ResetCursorBlink();
			PushUndo(UndoAction::Erase, false);
			DidChange();
			return true;
		}
		return false;
	}

	bool TextInput2::DeleteToEnd()
	{
		if (DeleteSelection())
			return true;

		if (Delete(_cursor, static_cast<int32_t>(_text.length()) - _cursor))
		{
			ResetCursorBlink();
			PushUndo(UndoAction::Erase, false);
			DidChange();
			return true;
		}
		return false;
	}

	bool TextInput2::DeleteSelection()
	{
		if (not HasSelection())
			return false;

		int32_t position, length;
		if (GetSelection(position, length) and Delete(position, length))
		{
			PushUndo(UndoAction::Erase, false);
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

		int textX = x - rect.x + _scroll.x;
		int textY = y - rect.y + _scroll.y;
		auto pos = GetCursorAt(textX, textY).position;
		
		if (IsShiftDown())
		{
			auto last_cursor = _cursor;
			SetCursor(pos);
			OnMoveCursor(last_cursor);
		}
		else
		{
			SetCursor(pos);
			Select(pos, -1);
			_bIsHighlighting = true;
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
			int textX = x - rect.x + _scroll.x;
			int textY = y - rect.y + _scroll.y;
			auto pos = GetCursorAt(textX, textY).position;

			SetCursor(pos);
			Select(highlight_start, _cursor);

			bHandled = true;
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

#pragma endregion Selection

	bool TextInput2::Copy()
	{
		if (_text.empty())
			return false;

		int32_t position, length;
		if (GetSelection(position, length))
		{
			char* temp = (char*)SDL_malloc(toUZ(length + 1));
			if (temp)
			{
				SDL_memcpy(temp, &_text[position], length);
				temp[length] = '\0';
				SDL_SetClipboardText(temp);
				SDL_free(temp);
			}
			return true;
		}
		return false;
	}

	bool TextInput2::Cut()
	{
		if (_text.empty())
			return false;

		int position, length;
		if (GetSelection(position, length))
		{
			char* temp = (char*)SDL_malloc(toUZ(length + 1));
			if (temp)
			{
				SDL_memcpy(temp, &_text[position], length);
				temp[length] = '\0';
				SDL_SetClipboardText(temp);
				SDL_free(temp);
			}

			if (Delete(position, length))
			{
				PushUndo(UndoAction::Erase, false);
				DidChange();
				return true;
			}
		}
		return false;
	}

	bool TextInput2::Paste()
	{
		fig::string content = SDL_GetClipboardText();
		if (content.empty())
			return false;
		normalize_newlines(content);

		if (!IsMultiline())
		{
			size_t pos_endl = index_of(content, 0, '\n');
			if (pos_endl != fig::npos)
				content.resize(pos_endl);
		}

		Insert(content);
		PushUndo(UndoAction::Write, false);
		DidChange();
		return true;
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
					_pOnEnter(_text); // Invoke
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
			return EventResult::Handled;

		case SDL_EVENT_TEXT_EDITING_CANDIDATES:
			ClearCandidates();
			SaveCandidates(&event); 
			return EventResult::Handled;

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
		if (_flags.IsSet({ Flag::WordWrap, Flag::Multi }))
		{
			int width = std::max(GetWidth() - GetMarginHorizontal(), 0);
			SetTextWrapWidth(width);
		}
		else
		{
			SetTextWrapWidth(0);
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
		_text.clear();
		_lines.clear();
		_cursor = 0;
		highlight_start = -1;
		highlight_end = -1;
		CancelComposition();

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
		int32_t lineSkip = TTF_GetFontLineSkip(_pFont);
		int32_t numRows = static_cast<int32_t>(GetLineCount());

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

		size_t length = SDL_utf8strlen(_text.c_str());

		wstring wdots;
		wdots.resize(length, L'\u2022');
		TTF_SetTextString(_pPassword, to_utf8(wdots).data(), 0);
	}

	int32_t TextInput2::ConvertToPasswordPosition(int32_t position) const
	{
		if (IsPassword())
		{
			int utf8_position = GetUtf8Codepoints(_text.c_str(), position);
			return Utf8CodepointsToBytes(_pPassword->text, utf8_position);
		}
		return position;
	}

	int32_t TextInput2::ConvertFromPasswordPosition(int32_t position) const
	{
		if (IsPassword())
		{
			int utf8_position = GetUtf8Codepoints(_pPassword->text, position);
			return Utf8CodepointsToBytes(_text.c_str(), utf8_position);
		}
		return position;
	}

#pragma region Undo/Redo

	TextInput2::UndoState TextInput2::GetUndoState(UndoAction action) const noexcept
	{
		return UndoState
		{
			.text = GetText(),
			.cursor_pos = _cursor,
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
		if (auto try_undo = _undo.Undo())
		{
			auto& undo = *try_undo;
			_text = undo.text;
			_lines = LayoutParagraph(_text);
			RefreshTexts();
			SetCursor(undo.cursor_pos);
			highlight_start = undo.highlight_start;
			highlight_end = undo.highlight_end;
		}
	}

	void TextInput2::Redo()
	{
		if (auto try_undo = _undo.Redo())
		{
			auto& undo = *try_undo;
			_text = undo.text;
			_lines = LayoutParagraph(_text);
			RefreshTexts();
			SetCursor(undo.cursor_pos);
			highlight_start = undo.highlight_start;
			highlight_end = undo.highlight_end;
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
			_pOnChanged(_text);

		OnText(_text);
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

	std::vector<TextInput2::TTFTextLine> TextInput2::LayoutParagraph(fig::string_view text)
	{
		std::vector<TTFTextLine> result;

		if (not IsMultiline())
		{
			size_t newlinePos = text.find('\n', 0);
			result.emplace_back(TTFTextLine {
				.position = 0,
				.length = static_cast<int32_t>(std::min(text.length(), newlinePos)),
				.eol = true,
			});
			return result;
		}

		if (not IsWordWrapping())
		{
			size_t paragraphStart = 0;
			while (paragraphStart < text.size())
			{
				size_t newlinePos = text.find('\n', paragraphStart);
				size_t paragraphEnd = (newlinePos == fig::string_view::npos) ? text.size() : newlinePos + 1uz;
				const char* pText = text.data() + paragraphStart;

				result.emplace_back(TTFTextLine {
					.position = static_cast<int32_t>(pText - text.data()),
					.length = static_cast<int32_t>(paragraphEnd - paragraphStart),
					.eol = true,
				});

				assert(result.back().length > 0);

				if (paragraphEnd >= text.size())
					break;

				paragraphStart = paragraphEnd;
			}
			return result;
		}

		size_t paragraphStart = 0;
		while (paragraphStart <= text.size())
		{
			size_t newlinePos = text.find('\n', paragraphStart);
			size_t paragraphEnd = (newlinePos == fig::string_view::npos) ? text.size() : newlinePos + 1uz;

			const char* pText = text.data() + paragraphStart;
			size_t remainingLength = paragraphEnd - paragraphStart;

			while (remainingLength > 0)
			{
				int32_t measuredWidth;
				size_t measuredLength;

				if (not TTF_MeasureString(_pFont, pText, remainingLength, _wrapWidth, &measuredWidth, &measuredLength))
					break;

				size_t breakWidth = measuredLength;
				if (measuredLength < remainingLength)
				{
					size_t lastSpace = measuredLength;

					while (lastSpace > 0 and not SDL_isspace(static_cast<unsigned char>(pText[lastSpace - 1])))
						--lastSpace;

					if (lastSpace > 0)
						measuredLength = lastSpace;
				}

				size_t advance = measuredLength;

				while (advance < remainingLength and SDL_isspace(static_cast<unsigned char>(pText[advance])) and pText[advance] != '\n')
					++advance;

				if (advance < remainingLength and pText[advance] == '\n')
					++advance;

				result.emplace_back(TTFTextLine {
					.position = static_cast<int32_t>(pText - text.data()),
					.length = static_cast<int32_t>(advance),
				});

				result.back().eol = IsEOL(result.back());

				pText += advance;
				remainingLength -= advance;
			}

			if (newlinePos == fig::string_view::npos)
				break;

			paragraphStart = paragraphEnd;
		}

		if (not result.empty())
			result.back().eol = true;

		return result;
	}

	TextInput2::TTFCursor TextInput2::GetCursorAt(int32_t position) const noexcept
	{
		if (_lines.empty())
			return {};

		if (position >= _text.size())
		{
			return TTFCursor {
				.position = position,
				.offset = position - _lines.back().position,
				.line = static_cast<int32_t>(_lines.size() - 1),
			};
		}

		for (size_t i = 0uz; i < _lines.size(); ++i)
		{
			auto& line = _lines[i];
			if (position >= line.position and position < line.position + line.length)
			{
				return TTFCursor {
					.position = position,
					.offset = position - line.position,
					.line = static_cast<int32_t>(i)
				};
			}
		}
		return {};
	}

	TextInput2::TTFCursor TextInput2::GetCursorAt(int32_t x, int32_t y) const noexcept
	{
		if (IsPassword())
		{
			TTF_SubString substring;
			if (TTF_GetTextSubStringForPoint(_pPassword.get(), x, _lineHeight / 2, &substring))
			{
				int32_t pos = GetCursorTextIndex(x, &substring);
				pos = ConvertFromPasswordPosition(pos);

				return TTFCursor {
					.position = pos,
					.offset = pos,
					.line = 0,
				};
			}
			return {};
		}

		if (not _lines.empty())
		{
			size_t line_index = static_cast<size_t>(std::clamp(y / _lineHeight, 0, static_cast<int32_t>(_lines.size() - 1)));
			auto& line = _lines[line_index];
			TTF_SubString substring;
			if (TTF_GetTextSubStringForPoint(line.ttf_text.get(), x, _lineHeight / 2, &substring))
			{
				int32_t pos = GetCursorTextIndex(x, &substring);
				return TTFCursor {
					.position = line.position + pos,
					.offset = pos,
					.line = static_cast<int32_t>(line_index),
				};
			}
		}

		return {};
	}

	TextInput2::TTFCursor TextInput2::GetLineCursor(size_t line_index) const noexcept
	{
		if (line_index < _lines.size())
		{
			auto& line = _lines[line_index];
			return TTFCursor {
				.position = line.position,
				.offset = 0,
				.line = static_cast<int32_t>(line_index)
			};
		}

		return {};
	}

	int32_t TextInput2::SetCursor(int32_t index) noexcept
	{
		if (_composition_length > 0)
		{
			/* Don't let the cursor be moved into the composition */
			if (index >= _composition_start and index <= (_composition_start + _composition_length))
				return _cursor;

			CancelComposition();
		}

		ResetCursorBlink();
		_cursor = std::clamp(index, 0, static_cast<int32_t>(_text.length()));
		return _cursor;
	}

	int32_t TextInput2::SetCursor(fig::point position) noexcept
	{
		SetCursor(GetCursorAt(position.x, position.y).position);
		return _cursor;
	}

	TextInput2::TTFCursor TextInput2::GetCursor() const noexcept
	{
		return GetCursorAt(_cursor);
	}

	void TextInput2::SelectAll() noexcept
	{
		highlight_start = 0;
		highlight_end = static_cast<int32_t>(_text.size());
	}

	void TextInput2::Select(int32_t start, int32_t end) noexcept
	{
		highlight_start = start;
		highlight_end = end;
	}

	void TextInput2::Deselect() noexcept
	{
		highlight_start = -1;
		highlight_end = -1;
	}

	bool TextInput2::GetSelection(int32_t& marker, int32_t& length) const noexcept
	{
		if (HasSelection())
		{
			auto start = std::min(highlight_start, highlight_end);
			auto end = std::max(highlight_start, highlight_end);
			marker = start;
			length = end - start;
			return true;
		}
		return false;
	}

	void TextInput2::Insert(int32_t position, fig::string_view text)
	{
		DeleteSelection();

		if (_text.empty())
		{
			_text = text;
			_lines = LayoutParagraph(_text);
			RefreshTexts();
			SetCursor(static_cast<int32_t>(text.size()));
			return;
		}

		auto cursor = GetCursorAt(position);
		int32_t paragraphStartLine = cursor.line;
		int32_t paragraphEndLine = cursor.line;

		while (not _lines[paragraphEndLine].eol and paragraphEndLine < static_cast<int32_t>(_lines.size()) - 1)
			++paragraphEndLine;

		int32_t paragraphStart = _lines[paragraphStartLine].position;
		int32_t paragraphEnd = _lines[paragraphEndLine].position + _lines[paragraphEndLine].length;

		_text.insert_range(_text.cbegin() + position, text);

		int32_t delta = static_cast<int32_t>(text.size());
		paragraphEnd += delta;

		fig::string_view paragraphText(_text.data() + paragraphStart, paragraphEnd - paragraphStart);
		std::vector<TTFTextLine> newLines = LayoutParagraph(paragraphText);

		for (auto& line : newLines)
			line.position += paragraphStart;

		for (size_t i = paragraphEndLine + 1; i < _lines.size(); ++i)
			_lines[i].position += delta;

		_lines.erase(_lines.begin() + paragraphStartLine, _lines.begin() + paragraphEndLine + 1);
		_lines.insert(_lines.begin() + paragraphStartLine, std::make_move_iterator(newLines.begin()), std::make_move_iterator(newLines.end()));

		RefreshTexts();
		SetCursor(position + delta);
	}

	void TextInput2::Insert(fig::string_view text)
	{
		Insert(std::clamp(_cursor, 0, static_cast<int32_t>(_text.size())), text);
	}

	bool TextInput2::Delete(int32_t from, int32_t length)
	{
		if (_text.empty() or length <= 0)
			return false;

		auto startCursor = GetCursorAt(from);
		auto endCursor = GetCursorAt(from + length);

		int32_t paragraphStartLine = startCursor.line;
		int32_t paragraphEndLine = endCursor.line;

		while (not _lines[paragraphEndLine].eol and paragraphEndLine < static_cast<int32_t>(_lines.size()) - 1)
			++paragraphEndLine;

		int32_t paragraphStart = _lines[paragraphStartLine].position;
		int32_t paragraphEnd = _lines[paragraphEndLine].position + _lines[paragraphEndLine].length;

		_text.erase(_text.cbegin() + from, _text.cbegin() + from + length);

		paragraphEnd -= length;

		fig::string_view paragraphText(_text.data() + paragraphStart, paragraphEnd - paragraphStart);
		std::vector<TTFTextLine> newLines = LayoutParagraph(paragraphText);

		if (newLines.empty() and false)
		{
			assert(false);
			newLines.emplace_back(TTFTextLine {
				.position = 0,
				.length = 0,
				.eol = true,
			});
		}

		for (auto& line : newLines)
			line.position += paragraphStart;

		for (size_t i = paragraphEndLine + 1; i < _lines.size(); ++i)
			_lines[i].position -= length;

		_lines.erase(_lines.begin() + paragraphStartLine, _lines.begin() + paragraphEndLine + 1);
		_lines.insert(_lines.begin() + paragraphStartLine,
			std::make_move_iterator(newLines.begin()),
			std::make_move_iterator(newLines.end()));

		RefreshTexts();
		SetCursor(from);
		Deselect();
		return true;
	}

	bool TextInput2::IsEOL(const TTFTextLine& line) const noexcept
	{
		return line.position >= 0 and line.position + line.length <= _text.length() and _text[line.position + line.length - 1uz] == '\n';
	}

	std::vector<fig::rectf> TextInput2::GetHighlights() const noexcept
	{
		if (not HasSelection())
			return {};

		auto start = std::min(highlight_start, highlight_end);
		auto end = std::max(highlight_start, highlight_end);

		std::vector<fig::rectf> highlights;

		if (IsPassword())
		{
			start = ConvertToPasswordPosition(start);
			end = ConvertToPasswordPosition(end);
			if (TTF_SubString** pHighlights = TTF_GetTextSubStringsForRange(_pPassword.get(), start, end - start, NULL))
			{
				for (int i = 0; pHighlights[i]; ++i)
				{
					auto highlight_rect = to_rectf(pHighlights[i]->rect);
					highlight_rect.w = std::max(highlight_rect.w, 3.0f);
					highlight_rect.y += 0;
					if (highlight_rect.x <= 1.0f)
					{
						highlight_rect.w += highlight_rect.x;
						highlight_rect.x = 0;
					}
					highlights.push_back(highlight_rect);
				}
			}
		}
		else
		{
			highlights.reserve(_lines.size());
			for (size_t iLine = 0uz; iLine < _lines.size(); ++iLine)
			{
				auto& line = _lines[iLine];
				if (line.ttf_text.empty())
					continue;
				if (end <= line.position or start >= line.position + line.length)
					continue;

				auto pos_start = std::max(line.position, start) - line.position;
				auto pos_end = std::min(line.position + line.length, end) - line.position;
				if (TTF_SubString** pHighlights = TTF_GetTextSubStringsForRange(line.ttf_text.get(), pos_start, pos_end - pos_start, NULL))
				{
					for (int i = 0; pHighlights[i]; ++i)
					{
						auto highlight_rect = to_rectf(pHighlights[i]->rect);
						highlight_rect.w = std::max(highlight_rect.w, 3.0f);
						highlight_rect.y += iLine * _lineHeight;
						if (highlight_rect.x <= 1.0f)
						{
							highlight_rect.w += highlight_rect.x;
							highlight_rect.x = 0;
						}
						highlights.push_back(highlight_rect);
					}
				}
			}
		}
		return highlights;
	}

	fig::rectf TextInput2::GetCursorRect() const noexcept
	{
		if (IsPassword())
		{
			auto pos = ConvertToPasswordPosition(_cursor);
			TTF_SubString substring;
			TTF_GetTextSubString(_pPassword.get(), pos, &substring);
			return rectf {
				.x = static_cast<float>(substring.rect.x),
				.y = 0,
				.w = 1.0f,
				.h = static_cast<float>(_lineHeight),
			};
		}

		if (IsOnLastNewLine())
		{
			return rectf {
				.x = 0,
				.y = static_cast<float>(_lineHeight * _lines.size()),
				.w = 1.0f,
				.h = static_cast<float>(_lineHeight),
			};
		}

		auto cursor = GetCursor();

		fig::rectf rect {
			.x = 0.0f,
			.y = cursor.line * static_cast<float>(_lineHeight),
			.w = 1.0f,
			.h = static_cast<float>(_lineHeight),
		};

		if (cursor.line <_lines.size())
		{
			auto& line = _lines[cursor.line];
			if (not line.ttf_text.empty())
			{
				int32_t cursor_pos = cursor.offset; 
				TTF_SubString substring;
				if (TTF_GetTextSubString(line.ttf_text.get(), cursor_pos, &substring))
				{
					rect.x = static_cast<float>(substring.rect.x);
					rect.h = std::max(rect.h, static_cast<float>(_lineHeight));
				}
			}
		}
		return rect;
	}

	void TextInput2::RelayoutAll()
	{
		std::vector<TTFTextLine> newLines;
		int32_t paragraphStart = 0;

		for (size_t i = 0; i < _lines.size(); ++i)
		{
			if (not _lines[i].eol)
				continue;

			int32_t paragraphEnd = _lines[i].position + _lines[i].length;
			fig::string_view paragraphText(_text.data() + paragraphStart, paragraphEnd - paragraphStart);

			std::vector<TTFTextLine> paragraphLines = LayoutParagraph(paragraphText);

			for (auto& line : paragraphLines)
				line.position += paragraphStart;

			newLines.insert(newLines.end(),
				std::make_move_iterator(paragraphLines.begin()),
				std::make_move_iterator(paragraphLines.end()));

			paragraphStart = paragraphEnd;
		}

		_lines = std::move(newLines);
		RefreshTexts();
	}

	void TextInput2::RefreshTexts() noexcept
	{
		// Create text objects
		for (auto& line : _lines)
		{
			if (line.ttf_text.empty())
			{
				assert(line.position >= 0 and line.length >= 0 and line.position + line.length <= _text.size());
				line.ttf_text = fig::sdl::Text(GetSDLTextEngine(), _pFont, _text.data() + line.position, line.length);
				TTF_SetTextWrapWhitespaceVisible(line.ttf_text.get(), true);
			}
		}

		if (IsPassword())
			UpdatePassword();
	}

	bool TextInput2::IsOnLastNewLine() const noexcept
	{
		// If the cursor is at the end of the string, and the last character is a line break,
		// treat it as a new line.
		return _cursor > 0
			and _cursor == _text.size()
			and _text.back() == '\n';
	}
}