#include <pch.h>
#include "gui/TextBase.h"

namespace fig::gui
{
	TextBase::TextBase(fig::text_engine_ptr pTextEngine, FontFace fontFace, double ptSize) :
		_pTextEngine { pTextEngine }
	{
		SetFont(fontFace, ptSize);
		InvalidateText();
	}
	
	void TextBase::SetTextWrapWidth(int32_t width)
	{
		if (_wrapWidth == width)
			return;

		_wrapWidth = std::max(width, 0);
		_bWordWrap |= _wrapWidth > 0;
		LayoutAll();
	}

	void TextBase::SetFont(FontFace fontFace, double ptSize) noexcept
	{
		if (_pFont = Fonts::GetFont(fontFace, ptSize))
		{
			_fontHeight = TTF_GetFontHeight(_pFont);
			_lineSkip = TTF_GetFontLineSkip(_pFont);

			LayoutAll();
		}
		else
		{
			_fontHeight = 0;
			_lineSkip = 0;
		}
		InvalidateText();
	}

	int32_t TextBase::GetTextWrapWidth() const noexcept
	{
		return _wrapWidth;
	}

	void TextBase::ClearText()
	{
		_text.clear();
		_lines.clear();
	}

	void TextBase::SetText(fig::string_view text)
	{
		if (text == _text)
			return; // No change

		_text = text;
		_styleSpans.clear();
		_lines = LayoutParagraph(_text);
		RefreshTexts();
	}

	void TextBase::SetStyledText(fig::string_view text, std::span<const StyleSpan> styleSpans)
	{
		_text = text;
		_styleSpans.assign(styleSpans.begin(), styleSpans.end());
		_lines = LayoutParagraph(_text);
		RefreshTexts();
	}
	
	std::vector<TextBase::TTFTextLine> TextBase::LayoutParagraph(fig::string_view text) const
	{
		std::vector<TTFTextLine> result;

		if (not (_bWordWrap and _wrapWidth > 0))
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

	bool TextBase::IsEOL(const TTFTextLine& line) const noexcept
	{
		return line.position >= 0 and line.position + line.length <= _text.length() and _text[line.position + line.length - 1uz] == '\n';
	}

	void TextBase::LayoutAll()
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

	void TextBase::RefreshTexts() noexcept
	{
		// Create text objects
		for (auto& line : _lines)
		{
			if (line.runs.empty())
			{
				assert(line.position >= 0 and line.length >= 0 and line.position + line.length <= _text.size());
				FinalizeLine(line);
			}
		}

		InvalidateText();
		OnRefreshedTexts();
	}

	void TextBase::FinalizeLine(TTFTextLine& line) const
	{
		line.runs.clear();

		int32_t offsetX = 0;

		for (const auto& range : StyleLine(line))
		{
			StyledTextRun run {
				.ttf_text = fig::sdl::Text(_pTextEngine, _pFont, _text.data() + range.position, range.length),
				.position = range.position,
				.length = range.length,
				.styleId = range.styleId,
				.offsetX = offsetX,
			};

			if (IsStyled())
			{
				auto& style = GetTextStyle(range.styleId);
				TTF_SetTextColor(run.ttf_text.get(), style.fgColor.r, style.fgColor.g, style.fgColor.b, style.fgColor.a);
				TTF_SetTextWrapWhitespaceVisible(run.ttf_text.get(), true);
			}

			int32_t w;
			TTF_GetTextSize(run.ttf_text.get(), &w, NULL);
			offsetX += w;

			line.runs.push_back(std::move(run));
		}
	}

	void TextBase::InvalidateText()
	{
		_bInvalidated = true;
	}

	void TextBase::SetDefaultStyle(fig::color_ref fgColor) noexcept
	{
		if (_styles.empty())
			_styles.resize(1uz);

		_styles[0] = std::move(TextStyle {
			.fgColor = fgColor,
		});
	}

	TextStyleId TextBase::AddStyle(fig::color_ref fgColor) noexcept
	{
		if (_styles.empty())
			_styles.resize(2uz);

		_styles.emplace_back(TextStyle {
			.fgColor = fgColor,
		});
		return _styles.size() - 1uz;
	}

	bool TextBase::ApplyStyle(TextStyleId styleId, int32_t position, int32_t length) noexcept
	{
		if (toUZ(styleId) >= _styles.size())
			return false;

		auto it = _styleSpans.begin();
		for (; it != _styleSpans.end(); ++it)
		{
			if (it->position > position)
				break;
		}
		_styleSpans.emplace(it, StyleSpan {
			.position = position,
			.length = length,
			.styleId = styleId,
		});
		return true;
	}

	std::vector<TextBase::StyledTextRun> TextBase::StyleLine(const TextBase::TTFTextLine& line) const
	{
		std::vector<StyledTextRun> result;
		int32_t pos_line_end = line.position + line.length;
		int32_t pos_cursor = line.position;

		for (const auto& span : _styleSpans)
		{
			int32_t pos_span_end = span.position + span.length;

			if (pos_span_end <= pos_cursor)
				continue;
			if (span.position >= pos_line_end)
				break;

			int32_t pos_run_start = std::max(span.position, pos_cursor);
			int32_t pos_run_end = std::min(pos_span_end, pos_line_end);

			if (pos_run_start > pos_cursor)
			{
				result.emplace_back(StyledTextRun {
					.position = pos_cursor,
					.length = pos_run_start - pos_cursor,
					.styleId = 0,
				});
			}

			result.emplace_back(StyledTextRun {
				.position = pos_run_start,
				.length = pos_run_end - pos_run_start,
				.styleId = span.styleId,
			});

			pos_cursor = pos_run_end;
		}

		if (pos_cursor < pos_line_end)
		{
			result.emplace_back(StyledTextRun {
				.position = pos_cursor,
				.length = pos_line_end - pos_cursor,
				.styleId = 0,
			});
		}

		return result;
	}

	const TextStyle& TextBase::GetTextStyle(TextStyleId styleId) const
	{
		assert(styleId < _styles.size());
		return _styles[styleId];
	}

	bool TextBase::TTF_GetTextSubString(const TTFTextLine& line, int32_t cursor, TTF_SubString* pSubstring) const
	{
		int32_t offset = 0;
		for (auto& run : line.runs)
		{
			if (run.ttf_text.empty())
				continue;

			if (::TTF_GetTextSubString(run.ttf_text.get(), cursor + offset, pSubstring))
				return true;
			offset += run.offsetX;
		}
		return false;
	}

	bool TextBase::TTF_GetTextSubStringForPoint(const TTFTextLine& line, int x, int y, TTF_SubString* pSubstring) const
	{
		int32_t offset = 0;
		for (auto& run : line.runs)
		{
			if (run.ttf_text.empty())
				continue;

			if (::TTF_GetTextSubStringForPoint(run.ttf_text.get(), x + offset, y, pSubstring))
				return true;
			offset += run.offsetX;
		}
		return false;
	}
}