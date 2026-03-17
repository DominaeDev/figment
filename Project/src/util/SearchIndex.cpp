#include <pch.h>
#include "util/SearchIndex.h"
#include "util/StringUtility.h"

using namespace fig::util;

namespace fig
{
	static fig::wstring normalize_search_term(const fig::string& text)
	{
		auto result = lcase(from_utf8(text));
		return strip_diacritics(std::move(result));
	}

	static fig::wstring normalize_search_term(const fig::wstring& text)
	{
		auto result = lcase(text);
		return strip_diacritics(std::move(result));
	}

	static std::vector<fig::wstring> filter_search(const fig::wstring& text)
	{
		static constexpr auto is_delimiter = [](wchar_t c) {
			return std::iswpunct(static_cast<wint_t>(c))
				or std::iswspace(static_cast<wint_t>(c))
				or std::iswcntrl(static_cast<wint_t>(c));
		};

		auto chunks = text
			| std::views::chunk_by([&](wchar_t a, wchar_t b) { return !is_delimiter(a) && !is_delimiter(b); })
			| std::views::transform([](auto&& range) {
				auto s = fig::wstring(std::ranges::begin(range), std::ranges::end(range));
				return normalize_search_term(s);
			})
			| std::ranges::to<std::vector>();

		return chunks
			| std::views::filter([&](auto& s) { return not empty_or_whitespace(s); })
			| std::ranges::to<std::vector>();
	}

	static const std::unordered_set<std::wstring> excluded_words = {
		L"a", L"an", L"the",
		L"aboard", L"about", L"above", L"across", L"after", L"against", L"along", L"amid",
		L"among", L"anti", L"around", L"as", L"at", L"before", L"behind", L"below", L"beneath",
		L"beside", L"besides", L"between", L"beyond", L"but", L"by", L"concerning", L"considering",
		L"despite", L"down", L"during", L"except", L"excepting", L"excluding", L"following",
		L"for", L"from", L"in", L"inside", L"into", L"like", L"minus", L"near", L"of", L"so",
		L"off", L"on", L"onto", L"opposite", L"outside", L"over", L"past", L"per",
		L"plus", L"regarding", L"round", L"save", L"since", L"than", L"through", L"to", L"toward", L"towards",
		L"under", L"underneath", L"unlike", L"until", L"up", L"upon", L"versus", L"via",
		L"with", L"within", L"without",
		L"this", L"that", L"these", L"those",
		L"me", L"my", L"mine", L"his", L"her", L"its", L"our", L"their", L"theirs",
		L"you", L"your", L"yours",
		L"whose", L"what", L"which",
		L"all", L"every", L"many", L"much", L"some", L"few", L"any", L"no",
		L"each", L"either", L"neither", L"more", L"less", L"fewer", L"fewest",
		L"afterwards", L"almost", L"already", L"also", L"always", L"anyway", L"away", L"close", L"downstairs", L"enough",
		L"ever", L"everywhere", L"far", L"hence", L"here", L"however", L"instead", L"just", L"later", L"likewise", L"lots",
		L"moreover", L"never", L"nevertheless", L"now", L"often", L"once", L"overseas", L"sometimes", L"then", L"there",
		L"today", L"tomorrow", L"tonight", L"twice", L"upstairs", L"yesterday", L"yet",
		L"and", L"or", L"how", L"when", L"why", L"who", L"where", L"if",
		L"actually", L"again", L"anymore", L"are", L"became", L"because", L"been", L"can",
		L"character", L"characters", L"did", L"didn", L"does", L"doesn", L"even", L"get", L"got",
		L"has", L"have", L"must", L"mustn", L"not", L"sigh", L"sighs", L"too", L"user",
		L"was", L"way", L"whatever", L"while", L"will", L"wouldn",
	};

	static std::unordered_set<std::wstring> distinct_words(std::wstring&& text)
	{
		// Find distinct words
		std::unordered_set<std::wstring> result;
		std::wstring current;

		auto flush = [&]() {
			if (!current.empty())
			{
				if (current.size() > 2 and !excluded_words.count(current))
					result.insert(current);
				current.clear();
			}
		};

		for (wchar_t ch : text)
		{
			if (std::iswalpha(ch))
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
		std::vector<std::wstring> index { words.begin(), words.end() };
		std::sort(index.begin(), index.end());

		// Merge with index
		std::vector<std::wstring> merged;
		merged.reserve(_index.size() + index.size());
		std::set_union(_index.begin(), _index.end(), index.begin(), index.end(), std::back_inserter(merged));
		_index = merged;
	}

	void SearchIndex::AddTerms(std::span<const fig::string> texts) noexcept
	{
		for (auto& t : texts)
			AddTerm(t);
	}

	bool SearchIndex::Match(const fig::string& search_string) const noexcept
	{
		// Filter the input string
		auto search_words = filter_search(from_utf8(search_string));

		for (auto& word : search_words)
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
			result += to_utf8(word);
			result += ' ';
		}
		return result;
	}

	size_t SearchIndex::Deserialize(const fig::string& value) noexcept
	{
		_index.clear();
		if (value.size() == 0)
			return 0;

		std::wistringstream stream(from_utf8(value));

		std::wstring word;
		while (stream >> word)
			_index.emplace_back(std::move(word));

		std::sort(_index.begin(), _index.end());
		return _index.size();
	}
}