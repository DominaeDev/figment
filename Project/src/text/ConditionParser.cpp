#include <pch.h>
#include <math.hpp>
#include "text/ConditionParser.h"

namespace fig
{
	constexpr const char* OpAnd = "and";
	constexpr const char* OpOr = "or";
	constexpr const char* OpNot = "not";
	constexpr const char* OpAlways = "always";
	constexpr const char* OpNever = "never";

	std::expected<ConditionPtr, ConditionParseError> ConditionParser::Parse(fig::string_view expression)
	{
		ConditionParser parser(expression);
		auto result = parser.ParseOr();
		if (parser._current.type != TokenType::End)
			return std::unexpected(ConditionParseError::ParseError);
		return result;
	}

	ConditionParser::ConditionParser(fig::string_view expression) :
		_cursor(expression.data()),
		_start(expression.data()),
		_end(expression.data() + expression.size()),
		_current {}
	{
		Advance();
	}

	void ConditionParser::Advance()
	{
		_current = NextToken();
	}

	static bool stricmp(const char* s, std::string_view word) noexcept
	{
		for (size_t i = 0; i < word.size(); ++i)
		{
			if (s[i] == '\0'
				or (word[i] == ' ' and std::isspace(static_cast<unsigned char>(s[i]) == 0))
				or std::tolower(static_cast<unsigned char>(s[i])) != std::tolower(static_cast<unsigned char>(word[i])))
				return false;
		}
		return true;
	}

	ConditionParser::Token ConditionParser::NextToken()
	{
		while (_cursor < _end and std::isspace(static_cast<unsigned char>(*_cursor)))
			++_cursor;

		size_t pos_token = static_cast<size_t>(_cursor - _start);

		if (_cursor >= _end)
			return { TokenType::End, pos_token };

		const char character = *_cursor;

		switch (character)
		{
			case '(':
				++_cursor;
				return { TokenType::LeftParen, pos_token };
			case ')':
				++_cursor;
				return { TokenType::RightParen, pos_token };
			case '"':
			{
				++_cursor;
				const char* start = _cursor;
				while (_cursor < _end && *_cursor != '"' && *_cursor != '\n' && *_cursor != '\r')
					++_cursor;
				if (_cursor >= _end || *_cursor != '"')
					return { TokenType::Error };

				fig::string text(start, static_cast<size_t>(_cursor - start));
				++_cursor;
				return { TokenType::String, pos_token, std::move(text) };
			}
			case '>':
				if (stricmp(_cursor, ">="))	// a >= b
				{
					_cursor += 2;
					return { TokenType::GreaterOrEqual, pos_token };
				}
				else
				{
					++_cursor;
					return { TokenType::GreaterThan, pos_token };
				}
			case '<':
				if (stricmp(_cursor, "<="))	// a <= b
				{
					_cursor += 2;
					return { TokenType::LessOrEqual, pos_token };
				}
				else if (stricmp(_cursor, "<>"))	// a <> b
				{
					_cursor += 2;
					return { TokenType::NotEqual, pos_token };
				}
				else
				{
					++_cursor;
					return { TokenType::LessThan, pos_token };
				}
			case '=':
				if (stricmp(_cursor, "=="))	// a == b
				{
					_cursor += 2;
					return { TokenType::EqualStrict, pos_token };
				}
				else  // a = b
				{
					++_cursor;
					return { TokenType::Equal, pos_token };
				}
			case '!':
				if (stricmp(_cursor, "!=="))	// a !== b
				{
					_cursor += 3;
					return { TokenType::NotEqualStrict, pos_token };
				}
				else if (stricmp(_cursor, "!="))	// a != b
				{
					_cursor += 2;
					return { TokenType::NotEqual, pos_token };
				}
				else if (stricmp(_cursor, "!~="))	// a !~= b
				{
					_cursor += 3;
					return { TokenType::NotEqualApprox, pos_token };
				}
				else
				{
					++_cursor;
					return { TokenType::Error, pos_token };
				}
			case '~':
				if (stricmp(_cursor, "~="))	// a ~= b
				{
					_cursor += 2;
					return { TokenType::EqualApprox, pos_token };
				}
				else
				{
					++_cursor;
					return { TokenType::Error, pos_token };
				}
			case 'g':
			case 'G':
				if (stricmp(_cursor, "gt ")) // a gt b
				{
					_cursor += 3;
					return { TokenType::GreaterThan, pos_token };
				}
				else if (stricmp(_cursor, "ge ")) // a ge b
				{
					_cursor += 3;
					return { TokenType::GreaterOrEqual, pos_token };
				}
			case 'l':
			case 'L':
				if (stricmp(_cursor, "lt "))	// a lt b
				{
					_cursor += 3;
					return { TokenType::LessThan, pos_token };
				}
				else if (stricmp(_cursor, "le "))	// a le b
				{
					_cursor += 3;
					return { TokenType::LessOrEqual, pos_token };
				}
			case 'e':
			case 'E':
				if (stricmp(_cursor, "eq "))	// a eq b
				{
					_cursor += 3;
					return { TokenType::Equal, pos_token };
				}
			case 'i':
			case 'I':
				if (stricmp(_cursor, "is "))	// a is b
				{
					_cursor += 3;
					return { TokenType::Equal, pos_token };
				}
				else if (stricmp(_cursor, "is not ")) // a is not b
				{
					_cursor += 7;
					return { TokenType::NotEqual, pos_token };
				}
			case 'n':
			case 'N':
				if (stricmp(_cursor, "neq "))	// a neq b
				{
					_cursor += 4;
					return { TokenType::NotEqual, pos_token };
				}
			default:
				break;
		}

		if (std::isdigit(static_cast<unsigned char>(character)))
		{
			const char* start = _cursor;
			char* parseEnd;
			fig::fixed value = toFixed(std::strtof(_cursor, &parseEnd));
			_cursor = parseEnd;

			if (_cursor + 1 < _end and (*_cursor == ':' or *_cursor == 'd') and trunc(value) == value) // 1:2, 1d6
			{
				const char* denomStart = _cursor + 1;
				char* denomEnd;
				fig::fixed denom = toFixed(std::strtof(denomStart, &denomEnd));
				if (denom >= 0_fp and trunc(denom) == denom) // b is integer
				{
					_cursor = denomEnd;
					return { TokenType::Probability, pos_token, fig::string(start, static_cast<size_t>(_cursor - start)) };
				}
			}

			return { TokenType::Number, pos_token, fig::string(start, static_cast<size_t>(_cursor - start)), value };
		}

		if (std::isalpha(static_cast<unsigned char>(character)) || character == '_')
		{
			const char* start = _cursor;
			while (_cursor < _end and (std::isalnum(static_cast<unsigned char>(*_cursor)) || *_cursor == '_' || *_cursor == '-' || *_cursor == ':' || *_cursor == '.'))
				++_cursor;

			fig::string text(start, static_cast<size_t>(_cursor - start));

			if (text == OpAnd)
				return { TokenType::And };
			if (text == OpOr)
				return { TokenType::Or };
			if (text == OpNot)
				return { TokenType::Not };
			if (text == OpAlways)
				return { TokenType::Always };
			if (text == OpNever)
				return { TokenType::Never };

			return { TokenType::Identifier, pos_token, std::move(text) };
		}

		_cursor = std::min(_cursor + 1, _end);
		return { TokenType::Error, pos_token };
	}

	std::expected<ConditionPtr, ConditionParseError> ConditionParser::ParseOr()
	{
		auto lhs = ParseAnd();
		if (!lhs || _current.type != TokenType::Or)
			return lhs;

		std::vector<ConditionPtr> children;
		children.emplace_back(std::move(lhs.value()));

		while (_current.type == TokenType::Or)
		{
			Advance();
			auto operand = ParseAnd();
			if (!operand)
				return std::unexpected(operand.error());
			children.emplace_back(std::move(operand.value()));
		}

		return std::make_unique<OrCondition>(std::move(children));
	}

	std::expected<ConditionPtr, ConditionParseError> ConditionParser::ParseAnd()
	{
		auto lhs = ParseNot();
		if (!lhs || _current.type != TokenType::And)
			return lhs;

		std::vector<ConditionPtr> children;
		children.emplace_back(std::move(lhs.value()));

		while (_current.type == TokenType::And)
		{
			Advance();
			auto operand = ParseNot();
			if (!operand)
				return std::unexpected(operand.error());
			children.emplace_back(std::move(operand.value()));
		}

		return std::make_unique<AndCondition>(std::move(children));
	}

	std::expected<ConditionPtr, ConditionParseError> ConditionParser::ParseNot()
	{
		if (_current.type != TokenType::Not)
			return ParseParentheses();

		Advance();
		auto operand = ParseNot();
		if (!operand)
			return std::unexpected(operand.error());
		return std::make_unique<NotCondition>(std::move(operand.value()));
	}

	std::expected<ConditionPtr, ConditionParseError> ConditionParser::ParseParentheses()
	{
		if (_current.type != TokenType::LeftParen)
			return ParseAtom();

		Advance();
		auto inner = ParseOr();
		if (_current.type != TokenType::RightParen)
			return std::unexpected(ConditionParseError::ExpectedParen);
		if (!inner)
			return std::unexpected(inner.error());

		Advance();
		return inner;
	}

	static std::optional<Fraction> parse_fraction(fig::string_view text, fig::string delimiters)
	{
		auto pos_delim = text.find_first_of(delimiters);
		if (pos_delim == fig::string_view::npos)
			return std::nullopt;

		auto count = text.substr(0, pos_delim);
		auto sides = text.substr(pos_delim + 1);
		if (count.empty() or sides.empty())
			return std::nullopt;

		auto try_count = string_to_int(count);
		auto try_sides = string_to_int(sides);
		if (not try_count or not try_sides or *try_count <= 0 or *try_sides <= 0)
			return std::nullopt;

		return Fraction { *try_count, *try_sides };
	}

	std::expected<ConditionPtr, ConditionParseError> ConditionParser::ParseAtom()
	{
		CompareOperand lhsValue;
		if (_current.type == TokenType::Number)
			lhsValue = _current.number;
		else if (_current.type == TokenType::Identifier)
			lhsValue = ContextLocator { _current.text };
		else if (_current.type == TokenType::String)
			lhsValue = std::move(_current.text);
		else if (_current.type == TokenType::Probability)
		{
			if (auto try_prob = parse_fraction(_current.text, ":"); try_prob.has_value() and (*try_prob).numerator > 0 and (*try_prob).denominator > 0) // a:b
			{
				Advance();
				return std::make_unique<RandomCondition>((*try_prob).numerator, (*try_prob).denominator);
			}
			else if (auto try_dice = parse_fraction(_current.text, "dD"); try_dice.has_value() and (*try_dice).numerator > 0 and (*try_dice).denominator > 0) // a:b
				lhsValue = *try_dice;
			else
				return std::unexpected(ConditionParseError::InvalidValue);
		}
		else if (_current.type == TokenType::Always)
		{
			Advance();
			return std::make_unique<AlwaysCondition>();
		}
		else if (_current.type == TokenType::Never)
		{
			Advance();
			return std::make_unique<NeverCondition>();
		}
		else
			return std::unexpected(ConditionParseError::InvalidValue);

		Advance();

		std::optional<CompareOperator> compareOperator;

		switch (_current.type)
		{
			case TokenType::Equal:
				compareOperator = CompareOperator::Equal;
				break;
			case TokenType::EqualStrict:
				compareOperator = CompareOperator::EqualStrict;
				break;
			case TokenType::EqualApprox:
				compareOperator = CompareOperator::EqualApprox;
				break;
			case TokenType::NotEqual:
				compareOperator = CompareOperator::NotEqual;
				break;
			case TokenType::NotEqualStrict:
				compareOperator = CompareOperator::NotEqualStrict;
				break;
			case TokenType::NotEqualApprox:
				compareOperator = CompareOperator::NotEqualApprox;
				break;
			case TokenType::LessThan:
				compareOperator = CompareOperator::LessThan;
				break;
			case TokenType::LessOrEqual:
				compareOperator = CompareOperator::LessOrEqual;
				break;
			case TokenType::GreaterThan:
				compareOperator = CompareOperator::GreaterThan;
				break;
			case TokenType::GreaterOrEqual:
				compareOperator = CompareOperator::GreaterOrEqual;
				break;
			default:
				break;
		}

		if (not compareOperator.has_value())
		{
			if (auto name = std::get_if<ContextLocator>(&lhsValue))
				return std::make_unique<FlagCondition>(std::move(*name));
			return std::unexpected(ConditionParseError::ExpectedIdentifier);
		}

		Advance();

		CompareOperand rhsValue;
		if (_current.type == TokenType::Number)
			rhsValue = _current.number;
		else if (_current.type == TokenType::Probability)
		{
			if (auto try_dice = parse_fraction(_current.text, "dD"); try_dice.has_value() and (*try_dice).numerator > 0 and (*try_dice).denominator > 0) // a:b
				rhsValue = *try_dice;
			else
				return std::unexpected(ConditionParseError::InvalidValue);
		}
		else if (_current.type == TokenType::Identifier)
		{
			rhsValue = ContextLocator { _current.text };
		}
		else if (_current.type == TokenType::String)
		{
			rhsValue = std::move(_current.text);
		}
		else
			return std::unexpected(ConditionParseError::InvalidValue);

		Advance();

		return std::make_unique<ComparisonCondition>(std::move(lhsValue), std::move(rhsValue), *compareOperator);
	}

	std::vector<ConditionParser::TokenSpan> ConditionParser::GetTokenSpans(fig::string_view expression)
	{
		std::vector<TokenSpan> spans;
		ConditionParser parser(expression);

		while (parser._current.type != TokenType::End)
		{
			size_t start = std::distance(expression.data(), parser._start);
			TokenType tokenType = parser._current.type;
			size_t tokenStart = parser._current.position;
			size_t tokenEnd = std::distance(expression.data(), parser._cursor);

			parser.Advance();

			spans.emplace_back(TokenSpan {
				.token = tokenType,
				.position = start + tokenStart,
				.length = static_cast<size_t>(tokenEnd - tokenStart),
			});
		}

		return spans;
	}
}