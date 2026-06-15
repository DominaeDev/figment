#ifndef TEXT_EVALUATOR_H__
#define TEXT_EVALUATOR_H__
#pragma once

#include "text/Context.h"

namespace fig
{
	enum class TextEvaluationOption
	{
		Unescape				= 1 << 0,
		CollapseWhitespace		= 1 << 1,
		CapitalizeFirst			= 1 << 2,
		CapitalizeSentences		= 1 << 3,
		FixPunctuation			= 1 << 4,
	};
	using TextEvaluationOptions = EnumFlags<TextEvaluationOption>;
	extern TextEvaluationOptions DefaultTextEvaluationOptions;

	[[nodiscard]] fig::string eval_text(const fig::string& source, const Context& context, TextEvaluationOptions options = DefaultTextEvaluationOptions) noexcept;
	[[nodiscard]] fig::string eval_text(fig::string&& source, const Context& context, TextEvaluationOptions options = DefaultTextEvaluationOptions) noexcept;

	template <is_string_like T> requires (not std::same_as<T, fig::string>)
		[[nodiscard]] fig::string eval_text(const T& source, const Context& context, TextEvaluationOptions options = DefaultTextEvaluationOptions) noexcept
	{
		return eval_text(fig::string { source }, context, options);
	}

}

#endif