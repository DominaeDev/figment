#include <pch.h>
#include "text/TextEvaluator.h"
#include "text/Condition.h"

namespace fig
{
	struct TextSpan
	{
		size_t begin {};
		size_t end {};
		inline constexpr size_t length() const noexcept { return end - begin; }
	};

	static size_t find_scope_end(const fig::string& text, size_t start, char open = '{', char close = '}', char escape = '\\')
	{
		if (empty_or_whitespace(text))
			return fig::npos;

		int32_t scope = 0;
		for (; start < text.size(); ++start)
		{
			auto c = text[start];
			if (c == escape)
			{
				++start; // Skip next
				continue;
			}
			if (c == open)
				++scope;
			else if (c == close)
			{
				if (--scope <= 0)
					return start;
			}
		}
		return -1;
	}

	static size_t find_within_scope(const fig::string& text, size_t start, char needle, char open = '{', char close = '}', char escape = '\\', int32_t depth = 0)
	{
		int32_t scope = 0;
		for (; start < text.size(); ++start)
		{
			auto c = text[start];
			if (c == escape)
			{
				++start; // Skip next
				continue;
			}
			if (c == open)
				++scope;
			else if (c == close)
				--scope;
			else if (c == needle && scope == depth)
				return start; // Found
		}
		return -1;
	}

	static fig::string& unescape(fig::string& text, char escape = '\\')
	{
		size_t pos_write = 0;

		for (size_t pos_read = 0; pos_read < text.size(); ++pos_read)
		{
			if (text[pos_read] != escape || pos_read + 1 >= text.size())
			{
				text[pos_write++] = text[pos_read];
				continue;
			}

			++pos_read;

			auto ch = text[pos_read];
			if (ch == escape)
			{
				text[pos_write++] = escape;
				continue;
			}
			switch (ch)
			{
				case '{':
				case '}':
				case '?':
				case ':':
				case '|':
					text[pos_write++] = ch;
					break;
				default:
					text[pos_write++] = escape;
					--pos_read;
					break;
			}
		}

		text.resize(pos_write);
		return text;
	}

	static fig::string& escape(fig::string& text, char escape = '\\')
	{
		fig::string result;
		result.reserve(text.size());

		for (char ch : text)
		{
			if (ch == escape)
			{
				result += escape;
				result += escape;
				continue;
			}

			switch (ch)
			{
				case '\n':
					result += escape;
					result += 'n';
					break;
				case '\t':
					result += escape;
					result += 't';
					break;
				case '\r':
					result += escape;
					result += 'r';
					break;
				case '{':
				case '}':
				case '?':
				case ':':
				case '|':
					result += escape;
					result += ch;
					break;
				default:
					result += ch;
					break;
			}
		}

		text = std::move(result);
		return text;
	}

	static fig::string& unescape_whitespace(fig::string& text, char escape = '\\')
	{
		size_t pos_write = 0;

		for (size_t pos_read = 0; pos_read < text.size(); ++pos_read)
		{
			if (text[pos_read] != escape || pos_read + 1 >= text.size())
			{
				text[pos_write++] = text[pos_read];
				continue;
			}

			++pos_read;

			auto ch = text[pos_read];
			if (ch == escape)
			{
				text[pos_write++] = escape;
				continue;
			}
			switch (ch)
			{
				case 't':
					text[pos_write++] = '\t';
					break;
				case 'r':
					text[pos_write++] = '\r';
					break;
				case 'n':
					text[pos_write++] = '\n';
					break;
				case '_':
					text[pos_write++] = ' ';
					break;
				default:
					text[pos_write++] = escape;
					--pos_read;
					break;
			}
		}

		text.resize(pos_write);
		return text;
	}

	static fig::string& collapse_whitespace(fig::string& text)
	{
		size_t pos_write = 0;
		bool lastWasSpace = false;

		for (size_t pos_read = 0; pos_read < text.size(); ++pos_read)
		{
			char ch = text[pos_read];

			if (ch == '\n')
			{
				text[pos_write++] = '\n';
				lastWasSpace = false;
			}
			else if (ch == '\r')
			{
				lastWasSpace = false;
			}
			else if (ch == ' ' || ch == '\t')
			{
				if (!lastWasSpace)
				{
					text[pos_write++] = ' ';
					lastWasSpace = true;
				}
			}
			else
			{
				text[pos_write++] = ch;
				lastWasSpace = false;
			}
		}

		text.resize(pos_write);
		return text;
	}

	static constexpr bool is_semantic_punctuation(char ch)
	{
		switch (ch)
		{
			case '.':
			case ',':
			case '!':
			case '?':
			case ';':
			case ')':
			case ']':
				return true;
			default:
				return false;
		}
	}

	fig::string& clean_punctuation(fig::string& text)
	{
		size_t pos_write = 0;

		for (size_t pos_read = 0; pos_read < text.size(); ++pos_read)
		{
			char ch = text[pos_read];

			if (ch == ' ' || ch == '\t')
			{
				size_t peek = pos_read + 1;

				while (peek < text.size() and (text[peek] == ' ' || text[peek] == '\t'))
					++peek;

				if (peek < text.size() and is_semantic_punctuation(text[peek]))
					continue;

				text[pos_write++] = ch;
			}
			else
			{
				text[pos_write++] = ch;
			}
		}

		text.resize(pos_write);
		return text;
	}

	static bool evaluate_condition(fig::string_view expression, const Context& context) noexcept
	{
		return Condition { fig::string { expression } }.Evaluate(context);
	}

	static bool substitute(fig::string& text, const TextSpan& span, const Context& context) noexcept
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
			bool r = evaluate_condition(condition, context);
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
			if (auto value = context.TryGetValue<fig::string>({ selector, value_key }))
			{
				text.replace(span.begin, span.length(), escape(value.value()));
				return true;
			}
		}
		
		text.erase(span.begin, span.length());
		return false;
	}

	static bool evaluate(fig::string& text, const Context& context) noexcept
	{
		// Find instances of "{...}"
		std::vector<TextSpan> spans;
		for (size_t pos = 0uz;;)
		{
			size_t pos_open = text.find('{', pos);
			if (pos_open == npos)
				break;

			if (pos_open > 0 && text[pos_open - 1] == '\\')
			{
				pos = pos_open + 1;
				continue; // Escaped
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
			substitute(text, span, context);

		return true;
	}

	fig::string TextEvaluator::Evaluate(const fig::string& source, const Context& context) noexcept
	{
		fig::string text { source };
		while (evaluate(text, context)) {};

		// Process result
		unescape_whitespace(collapse_whitespace(clean_punctuation(unescape(text))));
		return text;
	}

}