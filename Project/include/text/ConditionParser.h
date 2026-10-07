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
				
		static std::expected<ConditionPtr, ConditionParseError> Parse(fig::string_view expression);

		struct TokenSpan
		{
			TokenType token;
			size_t position;
			size_t length;
		};
		static std::vector<TokenSpan> GetTokenSpans(fig::string_view expression);
	private:
		struct Token
		{
			TokenType type = TokenType::End;
			size_t position {};
			fig::string text;
			fig::fixed number {};
		};

		explicit ConditionParser(fig::string_view expression);

		void Advance();
		Token NextToken();

		std::expected<ConditionPtr, ConditionParseError> ParseOr();
		std::expected<ConditionPtr, ConditionParseError> ParseAnd();
		std::expected<ConditionPtr, ConditionParseError> ParseNot();
		std::expected<ConditionPtr, ConditionParseError> ParseParentheses();
		std::expected<ConditionPtr, ConditionParseError> ParseAtom();

		const char* _cursor;
		const char* _start;
		const char* _end;
		Token _current {};
	};
}
