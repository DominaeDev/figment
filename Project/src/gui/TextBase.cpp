#include <pch.h>
#include "gui/TextBase.h"

namespace fig::gui
{
	TextBase::TextBase(fig::text_engine_ptr pTextEngine, FontFace fontFace, double ptSize) :
		_pTextEngine { pTextEngine }
	{
		_pFont = Fonts::GetFont(fontFace, ptSize);
		if (_pFont)
			_lineHeight = TTF_GetFontLineSkip(_pFont);
	}
	
	void TextBase::SetTextWrapWidth(int32_t width)
	{
		if (_wrapWidth == width)
			return;

		_wrapWidth = std::max(width, 0);
		LayoutAll();
	}

	void TextBase::SetFont(FontFace fontFace, double ptSize) noexcept
	{
		if (auto font = Fonts::GetFont(fontFace, ptSize))
		{
			_pFont = font;
			_lineHeight = TTF_GetFontLineSkip(_pFont);

			LayoutAll();
		}
	}

	int32_t TextBase::GetTextWrapWidth() const noexcept
	{
		return _wrapWidth;
	}

	int32_t TextBase::GetLineCount() const noexcept
	{
		int32_t count = toI(_lines.size());
		if (not _text.empty() and _text.back() == '\n')
			count++;
		return count;
	}

	void TextBase::ClearText()
	{
		_text.clear();
		_lines.clear();
	}

	void TextBase::SetText(fig::string_view text)
	{
		_text = text;
		_lines = LayoutParagraph(_text);
		RefreshTexts();
	}
	
	std::vector<TextBase::TTFTextLine> TextBase::LayoutParagraph(fig::string_view text)
	{
		std::vector<TTFTextLine> result;

		if (not _bMultiline)
		{
			size_t newlinePos = text.find('\n', 0);
			text = fig::string_view { text.data(), std::min(text.length(), newlinePos) };
		}

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
			if (line.ttf_text.empty())
			{
				assert(line.position >= 0 and line.length >= 0 and line.position + line.length <= _text.size());
				line.ttf_text = fig::sdl::Text(_pTextEngine, _pFont, _text.data() + line.position, line.length);
				TTF_SetTextWrapWhitespaceVisible(line.ttf_text.get(), true);
			}
		}
	}
}