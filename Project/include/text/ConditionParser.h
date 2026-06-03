#ifndef CONDITION_PARSER_H__
#define CONDITION_PARSER_H__
#pragma once

#include "ConditionNode.h"

namespace fig
{
	class ConditionParser
	{
	public:
		static std::expected<ConditionPtr, ConditionParseError> Parse(const fig::string& expression);

	private:
		enum class TokenType
		{
			End,
			Error,
			LeftParen,
			RightParen,
			And,
			Or,
			Not,
			Identifier,
			Number,
			Equal,
			NotEqual,
			LessThan,
			LessOrEqual,
			GreaterThan,
			GreaterOrEqual,
		};

		struct Token
		{
			TokenType type = TokenType::End;
			fig::string text;
			float number {};
		};

		explicit ConditionParser(const fig::string& source);

		void Advance();
		Token NextToken();

		std::expected<ConditionPtr, ConditionParseError> ParseOr();
		std::expected<ConditionPtr, ConditionParseError> ParseAnd();
		std::expected<ConditionPtr, ConditionParseError> ParseNot();
		std::expected<ConditionPtr, ConditionParseError> ParsePrimary();
		std::expected<ConditionPtr, ConditionParseError> ParseAtom();

		const char* _cursor;
		const char* _end;
		Token _current {};
	};
}
#endif