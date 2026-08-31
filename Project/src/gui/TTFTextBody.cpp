#include <pch.h>
#include "gui/TTFTextBody.h"

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

static int32_t FindPriorWord(const fig::string& text, int32_t cursor)
{
	const char* start = &text[cursor];
	const char* zero = &text[0];
	const char* curr = start;
	if (start == zero)
		return 0;

	enum CharType {
		Whitespace,
		Punctuation,
		Character,
		Invalid,
	};

	auto fnCharType = [](char ch) -> CharType {
		if (fig::is_whitespace(ch)) return CharType::Whitespace;
		if (fig::is_punctuation(ch)) return CharType::Punctuation;
		return CharType::Character;
	};

	auto fnMoveLeft = [text, &fnCharType](const char*& ch) -> CharType {
		if (SDL_StepBackUTF8(text.data(), &ch) == SDL_INVALID_UNICODE_CODEPOINT)
			return CharType::Invalid;
		return fnCharType(ch[0]);
	};

	auto fnMoveRight = [&fnCharType](const char*& ch) -> CharType {
		size_t length = SDL_strlen(ch);
		if (SDL_StepUTF8(&ch, &length) == SDL_INVALID_UNICODE_CODEPOINT)
			return CharType::Invalid;
		return fnCharType(ch[0]);
	};

	// Skip any whitespace first
	while (curr > zero)
	{
		CharType charType = fnMoveLeft(curr);
		if (not fig::is_whitespace(curr[0]))
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

static int32_t FindNextWord(const fig::string& text, int32_t cursor)
{
	size_t length = text.length();
	const char* start = &text[cursor];
	const char* end = &text[length];
	const char* curr = start;
	if (start == end)
		return cursor;

	enum CharType {
		Whitespace,
		Punctuation,
		Character,
	};

	auto fnCharType = [](char ch) -> CharType {
		if (fig::is_whitespace(ch)) return CharType::Whitespace;
		if (fig::is_punctuation(ch)) return CharType::Punctuation;
		return CharType::Character;
	};

	auto fnMoveLeft = [text, &fnCharType](const char*& ch) -> CharType {
		SDL_StepBackUTF8(text.data(), &ch);
		return fnCharType(ch[0]);
	};

	auto fnMoveRight = [&fnCharType](const char*& ch) -> CharType {
		size_t length = SDL_strlen(ch);
		SDL_StepUTF8(&ch, &length);
		return fnCharType(ch[0]);
	};

	CharType startType = fnCharType(start[0]);
	if (startType == CharType::Whitespace)
	{
		// On whitespace: only erase whitespace
		while (curr < end and fig::is_whitespace(curr[0]))
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
	TTFTextBody::TTFTextBody(fig::text_engine_ptr pTextEngine, fig::observer_ptr<TTF_Font> pFont) :
		_pTextEngine { pTextEngine },
		_pFont { pFont }
	{
		assert(pTextEngine);
		assert(pFont);
		_lineHeight = TTF_GetFontHeight(pFont);
	}

	void TTFTextBody::SetText(fig::string_view text)
	{
		Clear();
		Insert(text);
	}

	void TTFTextBody::SetTextWrapWidth(int32_t width)
	{
		if (_wrapWidth == width)
			return;

		_wrapWidth = std::max(width, 0);
	}

	int32_t TTFTextBody::GetTextWrapWidth() const noexcept
	{
		return _wrapWidth;
	}

	size_t TTFTextBody::GetLineCount() const noexcept
	{
		return _lines.size();
	}

	const std::vector<TTFTextLine>& TTFTextBody::GetLines() const noexcept
	{
		return _lines;
	}

	void TTFTextBody::Clear()
	{
		_text.clear();
		_lines.clear();
		_cursor = 0;
		highlight_start = -1;
		highlight_end = -1;
		_bInvalidated = false;
	}

	std::vector<TTFTextLine> TTFTextBody::LayoutParagraph(fig::string_view text)
	{
		std::vector<TTFTextLine> result;
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

	void TTFTextBody::Render(fig::renderer_ptr pRenderer)
	{
		if (_bInvalidated)
		{
			_bInvalidated = false;

			for (auto& line : _lines)
			{
				if (line.ttf_text.empty())
				{
					assert(line.position >= 0 and line.length >= 0 and line.position + line.length <= _text.size());
					line.ttf_text = fig::sdl::Text(_pTextEngine.get(), _pFont, _text.data() + line.position, line.length);
					TTF_SetTextWrapWhitespaceVisible(line.ttf_text.get(), true);
				}
			}
		}
	}

	TTFCursor TTFTextBody::GetCursorAt(int32_t position) const noexcept
	{
		if (_lines.empty())
		{
			return TTFCursor {
				.pText = nullptr,
				.position = 0,
				.offset = 0,
				.line = 0,
			};
		}

		if (position >= _text.size())
		{
			return TTFCursor {
				.pText = _lines.back().ttf_text.get(),
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
					.pText = line.ttf_text.get(),
					.position = position,
					.offset = position - line.position,
					.line = static_cast<int32_t>(i)
				};
			}
		}
		return TTFCursor { nullptr, 0, 0 };
	}

	TTFCursor TTFTextBody::GetCursorAt(int32_t x, int32_t y) const noexcept
	{
		size_t line_index = static_cast<size_t>(std::clamp(y / _lineHeight, 0, static_cast<int32_t>(_lines.size() - 1)));
		if (line_index < _lines.size())
		{
			auto& line = _lines[line_index];
			TTF_SubString substring;
			if (TTF_GetTextSubStringForPoint(line.ttf_text.get(), x, _lineHeight / 2, &substring))
			{
				int32_t pos = GetCursorTextIndex(x, &substring);
				return TTFCursor {
					.pText = line.ttf_text.get(),
					.position = line.position + pos,
					.offset = pos,
					.line = static_cast<int32_t>(line_index),
				};
			}
		}

		return TTFCursor { nullptr, 0, 0 };
	}

	TTFCursor TTFTextBody::GetLineCursor(size_t line_index) const noexcept
	{
		if (line_index < _lines.size())
		{
			auto& line = _lines[line_index];
			return TTFCursor {
				.pText = line.ttf_text.get(),
				.position = line.position,
				.offset = 0,
				.line = static_cast<int32_t>(line_index)
			};
		}

		return TTFCursor { nullptr, 0, 0 };
	}

	int32_t TTFTextBody::SetCursor(int32_t index) noexcept
	{
		_cursor = std::clamp(index, 0, static_cast<int32_t>(_text.length()));
		return _cursor;
	}

	bool TTFTextBody::SetCursor(fig::point position) noexcept
	{
		int32_t x = position.x;
		int32_t y = _lineHeight / 2;
		int32_t line = position.y / _lineHeight;
		if (line < 0 or line >= GetLineCount())
			return false;

		auto cursor = GetLineCursor(line);
		TTF_SubString substring;
		if (TTF_GetTextSubStringForPoint(cursor.pText, x, y, &substring))
		{
			SetCursor(GetCursorTextIndex(x, &substring));
			return true;
		}
		return false;
	}

	TTFCursor TTFTextBody::GetCursor() const noexcept
	{
		return GetCursorAt(_cursor);
	}

	int32_t TTFTextBody::MoveCursor(int direction) noexcept
	{
		auto cursor = GetCursor();
		if (direction < 0)
			return SetCursor(cursor.position - 1);
		else
			return SetCursor(cursor.position + 1);
	}

	int32_t TTFTextBody::MoveCursorLeft() noexcept
	{
		return MoveCursor(-1);
	}

	int32_t TTFTextBody::MoveCursorRight() noexcept
	{
		return MoveCursor(1);
	}

	int32_t TTFTextBody::MoveCursorUp() noexcept
	{
		auto cursor = GetCursor();
		if (cursor.line == 0uz)
			return _cursor;

		auto& curr_line = _lines[cursor.line];
		TTF_SubString substring;
		if (TTF_GetTextSubString(curr_line.ttf_text.get(), cursor.offset, &substring))
		{
			int32_t x = substring.rect.x;
			auto& prev_line = _lines[cursor.line - 1];
			if (TTF_GetTextSubStringForPoint(prev_line.ttf_text.get(), x, _lineHeight / 2, &substring))
				return SetCursor(prev_line.position + GetCursorTextIndex(x, &substring));
		}
		return _cursor;
	}

	int32_t TTFTextBody::MoveCursorDown() noexcept
	{
		auto cursor = GetCursor();
		if (cursor.line + 1uz >= _lines.size())
			return _cursor;

		auto& curr_line = _lines[cursor.line];
		TTF_SubString substring;
		if (TTF_GetTextSubString(curr_line.ttf_text.get(), cursor.offset, &substring))
		{
			int32_t x = substring.rect.x;
			auto& next_line = _lines[cursor.line + 1];
			if (TTF_GetTextSubStringForPoint(next_line.ttf_text.get(), x, _lineHeight / 2, &substring))
				return SetCursor(next_line.position + GetCursorTextIndex(x, &substring));
		}
		return _cursor;
	}

	int32_t TTFTextBody::MoveCursorBeginningOfLine() noexcept
	{
		if (_lines.empty())
			return 0;

		auto& line = _lines[GetCursor().line];
		return SetCursor(line.position);
	}

	int32_t TTFTextBody::MoveCursorEndOfLine() noexcept
	{
		if (_lines.empty())
			return 0;

		auto& line = _lines[GetCursor().line];
		return SetCursor(line.position + line.length - (IsEOL(line) ? 1 : 0));
	}

	int32_t TTFTextBody::MoveCursorToPriorWord() noexcept
	{
		return SetCursor(FindPriorWord(_text, _cursor));
	}

	int32_t TTFTextBody::MoveCursorToNextWord() noexcept
	{
		return SetCursor(FindNextWord(_text, _cursor));
	}

	void TTFTextBody::SelectAll() noexcept
	{
		highlight_start = 0;
		highlight_end = static_cast<int32_t>(_text.size());
	}

	void TTFTextBody::Select(int32_t start, int32_t end) noexcept
	{
		highlight_start = start;
		highlight_end = end;
	}

	void TTFTextBody::Deselect() noexcept
	{
		highlight_start = -1;
		highlight_end = -1;
	}

	bool TTFTextBody::GetSelection(int32_t& marker, int32_t& length) const noexcept
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

	void TTFTextBody::Insert(int32_t position, fig::string_view text)
	{
		DeleteSelection();

		if (_text.empty())
		{
			_text = text;
			_lines = LayoutParagraph(_text);
			_bInvalidated = true;
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

		_bInvalidated = true;
		SetCursor(position + delta);
	}

	void TTFTextBody::Insert(fig::string_view text)
	{
		Insert(std::clamp(_cursor, 0, static_cast<int32_t>(_text.size())), text);
	}

	bool TTFTextBody::Delete(int32_t from, int32_t length)
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

		if (newLines.empty())
		{
			newLines.emplace_back(TTFTextLine {
				.position = 0,
				.length = 0,
				.eol = true,
				.dirty = true,
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

		_bInvalidated = true;
		SetCursor(from);
		Deselect();
		return true;
	}

	bool TTFTextBody::Delete()
	{
		if (DeleteSelection())
			return true;

		const char* start = &_text[_cursor];
		const char* next = start;
		size_t length = SDL_strlen(next);
		SDL_StepUTF8(&next, &length);
		length = static_cast<size_t>((uintptr_t)next - (uintptr_t)start);

		return Delete(_cursor, static_cast<int32_t>(length));
	}

	bool TTFTextBody::DeleteSelection()
	{
		if (not HasSelection())
			return false;

		int32_t position, length;
		return GetSelection(position, length)
			and Delete(position, length);
	}

	bool TTFTextBody::Backspace()
	{
		if (DeleteSelection())
			return true;

		if (_cursor > 0)
		{
			const char* start = &_text[_cursor];
			const char* next = start;
			SDL_StepBackUTF8(_text.c_str(), &next);
			int length = (int)((uintptr_t)start - (uintptr_t)next);

			return Delete(_cursor - length, length);
		}
		return false;
	}

	bool TTFTextBody::BackspaceToPriorWord()
	{
		if (DeleteSelection())
			return true;

		int32_t prior = FindPriorWord(_text, _cursor);
		int32_t length = (_cursor - prior);
		return Delete(prior, length);
	}

	bool TTFTextBody::BackspaceToBeginning()
	{
		if (DeleteSelection())
			return true;

		return Delete(0, _cursor);
	}

	bool TTFTextBody::BackspaceToBeginningOfLine()
	{
		if (DeleteSelection())
			return true;

		auto cursor = GetCursor();
		return Delete(cursor.position, _cursor);
	}

	bool TTFTextBody::DeleteToNextWord()
	{
		if (DeleteSelection())
			return true;

		int32_t next = FindNextWord(_text, _cursor);
		int32_t length = next - _cursor;
		return Delete(_cursor, length);
	}

	bool TTFTextBody::DeleteToEndOfLine()
	{
		if (DeleteSelection())
			return true;

		auto cursor = GetCursor();
		if (cursor.line < _lines.size())
			return Delete(_cursor, _lines[cursor.line].position + _lines[cursor.line].length - cursor.position);
		return false;
	}

	bool TTFTextBody::DeleteToEnd()
	{
		if (DeleteSelection())
			return true;

		return Delete(_cursor, static_cast<int32_t>(_text.length()) - _cursor);
	}

	void TTFTextBody::Copy()
	{
		if (_text.empty())
			return;

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
		}
		else
		{
			SDL_SetClipboardText(_text.c_str());
		}
	}

	bool TTFTextBody::Cut()
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

			return Delete(position, length);
		}
		else
		{
//			SDL_SetClipboardText(_pText->text); //! @todo: Cut line??
//			TTF_DeleteTextString(_pText, 0, -1);
		}
		return false;
	}

	void TTFTextBody::Relayout()
	{
		return;
		/*
		while (true)
		{
			if (auto itFirst = std::ranges::find_if(_lines, [](auto&& l) { return l.dirty; }); itFirst != _lines.cend())
			{
				auto itLast = std::ranges::find_if(itFirst, _lines.end(), [this](auto&& l) { return l.eol; });
				if (itLast == _lines.end())
				{
					itLast = _lines.begin();
					std::advance(itLast, _lines.size() - 1uz);
				}

				auto position = (*itFirst).position;
				auto length = static_cast<size_t>((*itLast).position + (*itLast).length - (*itFirst).position);
				auto itInsert = _lines.erase(itFirst, ++itLast);

				auto newLines = LayoutParagraph(position, length);
				for (size_t i = 0uz; i < newLines.size(); ++i)
				{
					itInsert = _lines.insert(itInsert, std::move(newLines[i]));
					std::advance(itInsert, 1uz);
				}
				continue;
			}
			break;
		}

		for (auto& line : _lines)
		{
			if (line.ttf_text.empty())
			{
				assert(line.position >= 0 and line.length >= 0 and line.position + line.length <= _text.size());
				line.ttf_text = fig::sdl::Text(_pTextEngine.get(), _pFont, _text.data() + line.position, line.length);
				TTF_SetTextWrapWhitespaceVisible(line.ttf_text.get(), true);
			}
		}*/
	}

	fig::observer_ptr<TTF_Text> TTFTextBody::GetRenderedText() noexcept
	{
		if (not _lines.empty())
			return _lines[0].ttf_text.get(); //! @temp
		return nullptr;
	}

	bool TTFTextBody::IsEOL(const TTFTextLine& line) const noexcept
	{
		return line.position >= 0 and line.position + line.length <= _text.length() and _text[line.position + line.length - 1uz] == '\n';
	}

	std::vector<fig::rectf> TTFTextBody::GetHighlights() const noexcept
	{
		if (not HasSelection())
			return {};

		auto start = std::min(highlight_start, highlight_end);
		auto end = std::max(highlight_start, highlight_end);

		std::vector<fig::rectf> highlights;
		highlights.reserve(_lines.size());
		for (size_t iLine = 0uz; iLine < _lines.size(); ++iLine)
		{
			auto& line = _lines[iLine];
			if (line.ttf_text.empty())
				continue;
			if (end <= line.position or start >= line.position + line.length)
				continue;

			int32_t pos_start = std::max(line.position, start) - line.position;
			int32_t pos_end = std::min(line.position + line.length, end) - line.position;
			if (TTF_SubString** pHighlights = TTF_GetTextSubStringsForRange(line.ttf_text.get(), pos_start, pos_end - pos_start, NULL))
			{
				for (int i = 0; pHighlights[i]; ++i)
				{
					auto highlight_rect = to_rectf(pHighlights[i]->rect);
					highlight_rect.w = std::max(highlight_rect.w, 3.0f);
					highlight_rect.y += iLine * _lineHeight;
					if (highlight_rect.x <= 1.0f)
						highlight_rect.x = 0;
					highlights.push_back(highlight_rect);
				}
			}
		}
		return highlights;
	}

	fig::rectf TTFTextBody::GetCursorRect() const noexcept
	{
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
				TTF_SubString substring;
				if (TTF_GetTextSubString(line.ttf_text.get(), cursor.offset, &substring))
				{
					rect.x = static_cast<float>(substring.rect.x);
					rect.h = std::max(rect.h, static_cast<float>(_lineHeight));
				}
			}
		}
		return rect;
	}
}