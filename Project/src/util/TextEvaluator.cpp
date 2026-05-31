#include <pch.h>
#include "text/TextEvaluator.h"

namespace fig::text
{
	struct TextSpan
	{
		size_t begin {};
		size_t end {};
		inline constexpr size_t length() const noexcept { return end - begin; }
	};

	static bool _evaluate_condition(fig::string_view condition, const Contextual& context) noexcept
	{
		return true; //! @temp
	}

	static bool _substitute(fig::string& text, const TextSpan& span, const Contextual& context) noexcept
	{
		if (span.length() < 2)
			return false;

		fig::string expr = text.substr(span.begin + 1, span.length() - 2);
		if (empty_or_whitespace(expr))
		{
			text.erase(span.begin, span.length());
			return false;
		}

		size_t pos_selector = expr.find(':');
		size_t pos_cond = find_within_scope(expr, 0, '?');
		size_t pos_else = pos_cond != npos ? find_within_scope(expr, 0, '|') : npos;
		if (pos_cond != npos and pos_selector > pos_cond)
			pos_selector = npos;
		if (pos_cond != npos and pos_cond > pos_else)
			pos_else = npos;

		// Conditional?
		if (pos_cond != npos)
		{
			fig::string condition = trim(expr.substr(0, pos_cond));
			fig::string result_true, result_false;
			if (pos_else != npos)
			{
				result_true = expr.substr(pos_cond + 1, pos_else - pos_cond - 1);
				result_false = expr.substr(pos_else + 1);
			}
			else
			{
				result_true = expr.substr(pos_cond + 1);
			}
			bool r = _evaluate_condition(condition, context);
			text.replace(span.begin, span.length(), r ? result_true : result_false);
			return true;
		}

		// Value?
		Selector selector;
		fig::handle value_key;
		if (pos_selector != npos)
		{
			selector = Selector { trim(expr.substr(0, pos_selector)) };
			value_key = trim(expr.substr(pos_selector + 1));
		}
		else
		{
			value_key = trim(expr);
		}

		if (not value_key.empty())
		{
			if (auto try_ctx = context.TryGetContext(selector))
			{
				auto& ctx = try_ctx.value().get();
				if (auto value = ctx.TryGetValue<fig::string>(value_key))
				{
					text.replace(span.begin, span.length(), escape(value.value()));
					return true;
				}
			}
		}
		
		text.erase(span.begin, span.length());
		return false;
	}

	static bool _evaluate(fig::string& text, const Contextual& context) noexcept
	{
		// Find every instance of {...}
		std::vector<TextSpan> spans;
		for (size_t pos = 0uz;;)
		{
			size_t pos_open = text.find('{', pos);
			if (pos_open == npos)
				break;
			if (pos_open > 0 && text[pos_open - 1] == '\\')
			{
				pos = pos_open + 1;
				continue;
			}

			size_t pos_close = find_scope_end(text, pos_open);
			if (pos_close == npos)
				break;

			spans.emplace_back(TextSpan { pos_open, pos_close + 1 });
			pos = pos_close + 1;
		}

		if (spans.empty())
			return false;

		// Evaluate them
		for (auto& span : spans | std::views::reverse)
			_substitute(text, span, context);

		return true;
	}

	fig::string TextEvaluator::Evaluate(const fig::string& source, const Contextual& context) noexcept
	{
		fig::string text { source };
		while (_evaluate(text, context)) {};

		// Process result
		unescape_whitespace(collapse_whitespace(clean_punctuation(unescape(text))));
		return text;
	}


}