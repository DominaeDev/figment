#include <pch.h>
#include "text/ContextLocator.h"

namespace fig
{
	ContextSelector::ContextSelector(const char* name) : ContextSelector(fig::string(name))
	{
	}

	ContextSelector::ContextSelector(const fig::handle& name)
	{
		_keys.push_back(name);
	}

	ContextSelector::ContextSelector(const fig::string& name)
	{
		_keys.assign_range(name
			| std::ranges::views::split('.')
			| std::views::transform([](auto range) -> fig::handle {
			return fig::handle { std::string_view(range.data(), range.size()) };
		}));

		const auto [first, last] = std::ranges::remove_if(_keys, [](auto& h) { return h.empty(); });
		_keys.erase(first, last);
	}

	ContextSelector::operator fig::string() const noexcept
	{
		if (empty())
			return "";

		fig::string str;
		for (auto& key : _keys)
		{
			if (not str.empty())
				str.push_back('.');
			str.append((fig::string)key);
		}
		return str;
	}

	ContextSelector& ContextSelector::Append(const fig::handle& key) noexcept
	{
		if (not key.empty())
			_keys.push_back(key);
		return *this;
	}

	ContextLocator::operator fig::string() const noexcept
	{
		if (selector.empty())
			return (fig::string)key;
		else
			return std::format("{}:{}", (fig::string)selector, (fig::string)key);
	}
}