#include <pch.h>
#include "util/Dictionary.h"

namespace fig::util
{
	static constexpr auto is_delimiter = [](char c) {
		return std::ispunct(static_cast<unsigned char>(c))
			or std::isspace(static_cast<unsigned char>(c));
	};

	void Dictionary::AppendWords(const fig::string& text)
	{
		auto filtered_words = text
			| std::views::chunk_by([&](char a, char b) { return !is_delimiter(a) && !is_delimiter(b); })
			| std::views::filter([&](auto&& range) { return !is_delimiter(*std::ranges::begin(range)) && std::ranges::begin(range) != std::ranges::end(range); })
			| std::views::transform([](auto&& range) { auto s = fig::string(std::ranges::begin(range), std::ranges::end(range)); lcase_inplace(s); return s; });
		
		_words.insert_range(filtered_words);
	}

	void Dictionary::AppendWords(std::span<fig::string> words)
	{
		auto filtered_words = words
			| std::views::filter([](auto&& range) { return std::ranges::begin(range) != std::ranges::end(range); })
			| std::views::transform([](auto&& range) { auto s = fig::string(std::ranges::begin(range), std::ranges::end(range)); lcase_inplace(s); return s; });

		_words.insert_range(filtered_words);
	}

	bool Dictionary::Contains(const std::string& text) const noexcept
	{
		auto filtered_words = text
			| std::views::chunk_by([&](char a, char b) { return !is_delimiter(a) && !is_delimiter(b); })
			| std::views::filter([&](auto&& range) { return !is_delimiter(*std::ranges::begin(range)) && std::ranges::begin(range) != std::ranges::end(range); })
			| std::views::transform([](auto&& range) { auto s = fig::string(std::ranges::begin(range), std::ranges::end(range)); lcase_inplace(s); return s; })
			| std::ranges::to<std::vector>();

		for (auto& word : filtered_words)
			if (_words.contains(word))
				return true;
		return false;
	}
}