#ifndef TEXT_EVALUATOR_H__
#define TEXT_EVALUATOR_H__
#pragma once

#include "text/Context.h"

namespace fig
{
	struct TextEvaluator
	{
		[[nodiscard]] static fig::string Evaluate(const fig::string& source, const Context& context) noexcept;
		[[nodiscard]] static fig::string Evaluate(fig::string&& source, const Context& context) noexcept;

		template <is_string_like T> requires (not std::same_as<T, fig::string>)
			[[nodiscard]] static fig::string Evaluate(const T& source, const Context& context) noexcept
		{
			return Evaluate(fig::string { source }, context);
		}
	};
}

#endif