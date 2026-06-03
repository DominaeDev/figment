#include <pch.h>
#include "text/ConditionParser.h"

namespace fig
{
	constexpr const char* OpAnd = "and";
	constexpr const char* OpOr = "or";
	constexpr const char* OpNot = "not";

	std::expected<ConditionPtr, ConditionParseError> ConditionParser::Parse(const fig::string& expression)
	{
		ConditionParser parser(expression);
		auto result = parser.ParseOr();
		if (parser._current.type != TokenType::End)
			return std::unexpected(ConditionParseError::ParseError);
		return result;
	}

	ConditionParser::ConditionParser(const fig::string& source) :
		_cursor(source.data()),
		_end(source.data() + source.size()),
		_current {}
	{
		Advance();
	}

	void ConditionParser::Advance()
	{
		_current = NextToken();
	}

	ConditionParser::Token ConditionParser::NextToken()
	{
		while (_cursor < _end && std::isspace(static_cast<unsigned char>(*_cursor)))
			++_cursor;

		if (_cursor >= _end)
			return { TokenType::End };

		const char character = *_cursor;

		switch (character)
		{
			case '(':
				++_cursor;
				return { TokenType::LeftParen };
			case ')':
				++_cursor;
				return { TokenType::RightParen };
			case '>':
				if (_cursor + 1 < _end && _cursor[1] == '=')	// a >= b
				{
					_cursor += 2;
					return { TokenType::GreaterOrEqual };
				}
				++_cursor;
				return { TokenType::GreaterThan };
			case '<':
				if (_cursor + 1 < _end && _cursor[1] == '=')	// a <= b
				{
					_cursor += 2;
					return { TokenType::LessOrEqual };
				}
				if (_cursor + 1 < _end && _cursor[1] == '>')	// a <> b
				{
					_cursor += 2;
					return { TokenType::NotEqual };
				}
				++_cursor;
				return { TokenType::LessThan };
			case '=':											// a = b
				++_cursor;
				return { TokenType::Equal };
			case '!':
				if (_cursor + 1 < _end && _cursor[1] == '=')	// a != b
				{
					_cursor += 2;
					return { TokenType::NotEqual };
				}
				return { TokenType::Error };
			case 'g':
				if (_cursor + 1 < _end && _cursor[1] == 't')	// a gt b
				{
					_cursor += 2;
					return { TokenType::GreaterThan };
				}
				if (_cursor + 1 < _end && _cursor[1] == 'e')	// a ge b
				{
					_cursor += 2;
					return { TokenType::GreaterOrEqual };
				}
			case 'l':
				if (_cursor + 1 < _end && _cursor[1] == 't')	// a lt b
				{
					_cursor += 2;
					return { TokenType::LessThan };
				}
				if (_cursor + 1 < _end && _cursor[1] == 'e')	// a le b
				{
					_cursor += 2;
					return { TokenType::LessOrEqual };
				}
			case 'e':
				if (_cursor + 1 < _end && _cursor[1] == 'q')	// a eq b
				{
					_cursor += 2;
					return { TokenType::Equal };
				}
			case 'i':
				if (_cursor + 1 < _end && _cursor[1] == 's')	// a is b
				{
					_cursor += 2;
					return { TokenType::Equal };
				}
			case 'n':
				if (_cursor + 2 < _end && _cursor[1] == 'e' && _cursor[2] == 'q')	// a neq b
				{
					_cursor += 3;
					return { TokenType::NotEqual };
				}
			default:
				break;
		}

		if (std::isdigit(static_cast<unsigned char>(character)))
		{
			const char* start = _cursor;
			char* parseEnd;
			float value = std::strtof(_cursor, &parseEnd);
			_cursor = parseEnd;
			return { TokenType::Number, fig::string(start, static_cast<size_t>(_cursor - start)), value };
		}

		if (std::isalpha(static_cast<unsigned char>(character)) || character == '_')
		{
			const char* start = _cursor;
			while (_cursor < _end && (std::isalnum(static_cast<unsigned char>(*_cursor)) || *_cursor == '_' || *_cursor == ':' || *_cursor == '.'))
				++_cursor;

			fig::string text(start, static_cast<size_t>(_cursor - start));

			if (text == OpAnd)
				return { TokenType::And };
			if (text == OpOr)
				return { TokenType::Or };
			if (text == OpNot)
				return { TokenType::Not };

			return { TokenType::Identifier, std::move(text) };
		}

		return { TokenType::Error };
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
			return ParsePrimary();

		Advance();
		auto operand = ParseNot();
		if (!operand)
			return std::unexpected(operand.error());
		return std::make_unique<NotCondition>(std::move(operand.value()));
	}

	std::expected<ConditionPtr, ConditionParseError> ConditionParser::ParsePrimary()
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

	std::expected<ConditionPtr, ConditionParseError> ConditionParser::ParseAtom()
	{
		if (_current.type != TokenType::Identifier)
			return std::unexpected(ConditionParseError::ExpectedIdentifier);

		fig::string name = std::move(_current.text);
		Advance();

		std::optional<CompareOperator> compareOperator;

		switch (_current.type)
		{
			case TokenType::Equal:
				compareOperator = CompareOperator::Equal;
				break;
			case TokenType::NotEqual:
				compareOperator = CompareOperator::NotEqual;
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

		if (!compareOperator.has_value())
			return std::make_unique<FlagCondition>(std::move(name));

		Advance();

		RhsValue rhsValue;
		if (_current.type == TokenType::Number)
			rhsValue = _current.number;
		else if (_current.type == TokenType::Identifier)
			rhsValue = std::move(_current.text);
		else
			return std::unexpected(ConditionParseError::InvalidValue);

		Advance();

		return std::make_unique<ComparisonCondition>(std::move(name), *compareOperator, std::move(rhsValue));
	}
}