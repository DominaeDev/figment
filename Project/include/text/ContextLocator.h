#pragma once

#include "Figment.h"

namespace fig
{
	struct ContextSelector
	{
		ContextSelector() = default;
		ContextSelector(const char* name);
		ContextSelector(const fig::handle& name);
		ContextSelector(const fig::string& name);
		ContextSelector(const ContextSelector&) = default;
		ContextSelector(ContextSelector&&) = default;
		~ContextSelector() = default;

		ContextSelector& operator=(const ContextSelector&) = default;
		ContextSelector& operator=(ContextSelector&&) = default;

		auto& operator[](size_t index) const { return _keys[index]; }
		size_t size() const noexcept { return _keys.size(); }
		bool empty() const noexcept { return _keys.empty(); }
		operator bool() const noexcept { return not _keys.empty(); }

		explicit operator fig::string() const noexcept;

		ContextSelector& Append(const fig::handle& key) noexcept;

		static ContextSelector FromRole(fig::chat::Role role) noexcept;

	private:
		std::vector<fig::handle> _keys;
	};

	struct ContextLocator
	{
		ContextLocator() = default;
		ContextLocator(const ContextLocator& other) = default;
		ContextLocator(const ContextSelector& selector, const fig::handle& key = {})
		{
			this->selector = selector;
			this->key = key;
		}

		ContextLocator(const is_string_like auto& location)
		{
			fig::string s { location };
			if (size_t pos_selector = s.find(':'); pos_selector != npos)
			{
				selector = ContextSelector { trim(s.substr(0, pos_selector)) };
				key = fig::handle { trim(s.substr(pos_selector + 1)) };
			}
			else
			{
				key = fig::handle { trim(s) };
			}
		}

		explicit operator fig::string() const noexcept;

		operator bool() const noexcept { return not key.empty(); }

		ContextSelector selector;
		fig::handle key;
	};
}
