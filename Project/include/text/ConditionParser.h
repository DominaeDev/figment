#pragma once

#include "ConditionNode.h"

namespace fig
{
	enum class ConditionParseError
	{
		NoError,
		ParseError,
		UnexpectedToken,
		ExpectedIdentifier,
		ExpectedParen,
		InvalidValue,
	};

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
			String,
			Probability,
			Always,
			Never,
			Equal,
			EqualStrict,
			EqualApprox,
			NotEqual,
			NotEqualStrict,
			NotEqualApprox,
			LessThan,
			LessOrEqual,
			GreaterThan,
			GreaterOrEqual,
		};

		struct Token
		{
			TokenType type = TokenType::End;
			fig::string text;
			fig::fixed number {};
		};

		explicit ConditionParser(const fig::string& source);

		void Advance();
		Token NextToken();

		std::expected<ConditionPtr, ConditionParseError> ParseOr();
		std::expected<ConditionPtr, ConditionParseError> ParseAnd();
		std::expected<ConditionPtr, ConditionParseError> ParseNot();
		std::expected<ConditionPtr, ConditionParseError> ParseParentheses();
		std::expected<ConditionPtr, ConditionParseError> ParseAtom();

		const char* _cursor;
		const char* _end;
		Token _current {};
	};
}
