#include <pch.h>
#include "util/SearchIndex.h"
#include "util/StringUtility.h"
#include "util/ExcludedSearchTerms.h"

using namespace fig::util;

namespace fig
{
	static fig::string normalize_search_term(fig::string&& text)
	{
		text = lcase_inplace(text);
		return strip_diacritics(std::move(text));
	}

	static fig::string normalize_search_term(const fig::string& text)
	{
		auto ltext = lcase(text);
		return strip_diacritics(std::move(ltext));
	}

	static std::vector<fig::string> filter_search(const fig::string& text)
	{
		static constexpr auto is_delimiter = [](char c) {
			return std::ispunct(static_cast<int>(c))
				or std::isspace(static_cast<int>(c))
				or std::iscntrl(static_cast<int>(c));
		};

		auto chunks = text
			| std::views::chunk_by([&](char a, char b) { return !is_delimiter(a) && !is_delimiter(b); })
			| std::views::transform([](auto&& range) {
				auto s = fig::string(std::ranges::begin(range), std::ranges::end(range));
				return normalize_search_term(std::move(s));
			})
			| std::ranges::to<std::vector>();

		return chunks
			| std::views::filter([&](auto& s) { return not empty_or_whitespace(s); })
			| std::ranges::to<std::vector>();
	}

	SearchQuery::SearchQuery(const fig::string& query)
	{
		terms = filter_search(query);
	}

	static std::unordered_set<std::string> distinct_words(std::string&& text)
	{
		// Find distinct words
		std::unordered_set<std::string> result;
		std::string current;

		auto flush = [&]() {
			if (!current.empty())
			{
				if (current.size() >= 3 and !ExcludedSearchTerms.contains(current))
					result.insert(current);
				current.clear();
			}
		};

		for (char ch : text)
		{
			if (std::isalpha(ch))
				current += ch;
			else
				flush();
		}
		flush(); // Handle the final word

		return result;
	}

	void SearchIndex::AddTerm(const fig::string& text) noexcept
	{
		if (empty_or_whitespace(text))
			return;

		// Break up into individual, distinct words
		auto words = distinct_words(normalize_search_term(text));
		std::vector<std::string> index { words.begin(), words.end() };
		std::sort(index.begin(), index.end());

		// Merge with index
		std::vector<std::string> merged;
		merged.reserve(_index.size() + index.size());
		std::set_union(_index.begin(), _index.end(), index.begin(), index.end(), std::back_inserter(merged));
		_index = merged;
	}

	void SearchIndex::AddTerms(std::span<const fig::string> texts) noexcept
	{
		for (auto& t : texts)
			AddTerm(t);
	}

	bool SearchIndex::Match(const SearchQuery& query) const noexcept
	{
		for (auto& word : query.terms)
		{
			auto it = std::ranges::lower_bound(_index, word);
			if (it == _index.end() or not it->starts_with(word))
				return false;
		}
		return true;
	}

	fig::string SearchIndex::Serialize() const noexcept
	{
		fig::string result;
		result.reserve(_index.size() * 8);

		for (const auto& word : _index)
		{
			result += word;
			result += ' ';
		}
		return result;
	}

	size_t SearchIndex::Deserialize(const fig::string& value) noexcept
	{
		_index.clear();
		if (value.size() == 0)
			return 0;

		std::istringstream stream(value);

		std::string word;
		while (stream >> word)
			_index.emplace_back(std::move(word));

		std::sort(_index.begin(), _index.end());
		return _index.size();
	}
}