#include <pch.h>
#include "gui/DefaultTextStyleProvider.h"
#include "text/ConditionParser.h"

namespace fig::gui
{
	using Token = ConditionParser::TokenType;
	constexpr TextStyleId kStyleDefault = 0;
	constexpr TextStyleId kStyleBraces = 1;
	constexpr TextStyleId kStyleVariable = 2;
	constexpr TextStyleId kStyleLiteral = 3;
	constexpr TextStyleId kStyleKeyword = 4;
	constexpr TextStyleId kStyleOperator = 5;

	static bool IsIdentifierStart(char c)
	{
		return SDL_isalpha(static_cast<unsigned char>(c)) or c == '_';
	}

	static bool IsIdentifierChar(char c)
	{
		return SDL_isalnum(static_cast<unsigned char>(c)) or c == '_' or c == '-';
	}

	static std::optional<size_t> FindMatchingBrace(fig::string_view text, size_t openPos)
	{
		int32_t depth = 1;
		size_t pos = openPos + 1;

		while (pos < text.size())
		{
			char c = text[pos];

			if (c == '\\' and pos + 1 < text.size() and (text[pos + 1] == '{' or text[pos + 1] == '}'))
			{
				pos += 2;
				continue;
			}

			if (c == '{')
				++depth;
			else if (c == '}')
			{
				--depth;
				if (depth == 0)
					return pos;
			}
			else if (c == '\r' or c == '\n')
			{
				return std::nullopt;
			}

			++pos;
		}

		return std::nullopt;
	}

	static size_t FindConditionSeparator(fig::string_view text, size_t start, size_t end)
	{
		int32_t depth = 0;
		bool inQuote = false;
		size_t pos = start;

		while (pos < end)
		{
			char c = text[pos];

			if (inQuote)
			{
				if (c == '\\' and pos + 1 < end)
					pos += 2;
				else
				{
					if (c == '"')
						inQuote = false;
					++pos;
				}
				continue;
			}

			if (c == '"')
			{
				inQuote = true;
				++pos;
				continue;
			}

			if (c == '\\' and pos + 1 < end and (text[pos + 1] == '{' or text[pos + 1] == '}'))
			{
				pos += 2;
				continue;
			}

			if (c == '{')
				++depth;
			else if (c == '}')
				--depth;
			else if (c == '?' and depth == 0)
				return pos;

			++pos;
		}

		return end;
	}

	static size_t FindBranchSeparator(fig::string_view text, size_t start, size_t end)
	{
		int32_t depth = 0;
		size_t pos = start;

		while (pos < end)
		{
			char c = text[pos];

			if (c == '\\' and pos + 1 < end and (text[pos + 1] == '{' or text[pos + 1] == '}'))
			{
				pos += 2;
				continue;
			}

			if (c == '{')
				++depth;
			else if (c == '}')
				--depth;
			else if (c == '|' and depth == 0)
				return pos;

			++pos;
		}

		return end;
	}

	static TextStyleId GetStyleForTokenType(ConditionParser::TokenType type)
	{
		switch (type)
		{
		case ConditionParser::TokenType::LeftParen:
		case ConditionParser::TokenType::RightParen:
			return kStyleBraces;
		case ConditionParser::TokenType::And:
		case ConditionParser::TokenType::Or:
		case ConditionParser::TokenType::Not:
		case ConditionParser::TokenType::Equal:
		case ConditionParser::TokenType::EqualStrict:
		case ConditionParser::TokenType::EqualApprox:
		case ConditionParser::TokenType::NotEqual:
		case ConditionParser::TokenType::NotEqualStrict:
		case ConditionParser::TokenType::NotEqualApprox:
		case ConditionParser::TokenType::LessThan:
		case ConditionParser::TokenType::LessOrEqual:
		case ConditionParser::TokenType::GreaterThan:
		case ConditionParser::TokenType::GreaterOrEqual:
			return kStyleOperator;
		case ConditionParser::TokenType::Always:
		case ConditionParser::TokenType::Never:
			return kStyleKeyword;
		case ConditionParser::TokenType::Number:
		case ConditionParser::TokenType::String:
		case ConditionParser::TokenType::Probability:
			return kStyleLiteral;
		case ConditionParser::TokenType::Identifier:
			return kStyleVariable;
		default:
			return kStyleDefault;
		}
	}

	void ParseCondition(fig::string_view text, size_t start, size_t end, std::vector<StyleSpan>& result)
	{
		fig::string conditionText(text.substr(start, end - start));
		auto tokens = ConditionParser::GetTokenSpans(conditionText);

		for (const auto& token : tokens)
		{
			result.emplace_back(StyleSpan {
				.position = static_cast<int32_t>(token.position + start),
				.length = static_cast<int32_t>(token.length),
				.styleId = GetStyleForTokenType(token.token),
			});
		}
	}

	void Parse(fig::string_view text, size_t start, size_t end, std::vector<StyleSpan>& result)
	{
		size_t pos = start;

		while (pos < end)
		{
			char c = text[pos];

			if (c == '\\' and pos + 1 < end and (text[pos + 1] == '{' or text[pos + 1] == '}'))
			{
				pos += 2;
				continue;
			}

			if (c != '{')
			{
				++pos;
				continue;
			}

			auto closePos = FindMatchingBrace(text, pos);

			if (not closePos or *closePos >= end)
			{
				++pos;
				continue;
			}

			size_t openPos = pos;
			size_t contentStart = openPos + 1;
			size_t contentEnd = *closePos;
			size_t separatorPos = FindConditionSeparator(text, contentStart, contentEnd);

			result.emplace_back(StyleSpan {
				.position = static_cast<int32_t>(openPos),
				.length = 1,
				.styleId = kStyleBraces,
			});

			if (separatorPos == contentEnd)
			{
				result.emplace_back(StyleSpan {
					.position = static_cast<int32_t>(contentStart),
					.length = static_cast<int32_t>(contentEnd - contentStart),
					.styleId = kStyleVariable,
				});
			}
			else
			{
				ParseCondition(text, contentStart, separatorPos, result);

				result.emplace_back(StyleSpan {
					.position = static_cast<int32_t>(separatorPos),
					.length = 1,
					.styleId = kStyleOperator,
				});

				size_t branchStart = separatorPos + 1;
				size_t pipePos = FindBranchSeparator(text, branchStart, contentEnd);

				Parse(text, branchStart, pipePos, result);

				if (pipePos != contentEnd)
				{
					result.emplace_back(StyleSpan {
						.position = static_cast<int32_t>(pipePos),
						.length = 1,
						.styleId = kStyleOperator,
					});
					Parse(text, pipePos + 1, contentEnd, result);
				}
			}

			result.emplace_back(StyleSpan { 
				.position = static_cast<int32_t>(contentEnd), 
				.length = 1, 
				.styleId = kStyleBraces 
			});

			pos = contentEnd + 1;
		}
	}

	DefaultTextStyleProvider::DefaultTextStyleProvider(TextBase* pOwner)
	{
		AddStyle(Color::TextBoxForeground);		// kStyleDefault
		AddStyle(Color::CommandBrace);			// kStyleBraces
		AddStyle(Color::CommandValue);			// kStyleVariable
		AddStyle(Color::CommandLiteral);		// kStyleLiteral
		AddStyle(Color::CommandKeyword);		// kStyleKeyword
		AddStyle(Color::CommandOperator);		// kStyleOperator
	}

	std::vector<StyleSpan> DefaultTextStyleProvider::GetStyles(fig::string_view text, size_t offset)
	{
		std::vector<StyleSpan> styles;
		Parse(text, 0, text.size(), styles);
		for (auto& style : styles)
			style.position += static_cast<int32_t>(offset);
		return styles;
	}
}