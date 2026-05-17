#ifndef DICTIONARY_H__
#define DICTIONARY_H__
#pragma once

#include "Figment.h"
#include <string>
#include <string_view>
#include <unordered_set>
#include <ranges>

namespace fig::util
{
	class Dictionary
	{
	public:
		void AppendWords(const std::string& text);
		void AppendWords(std::span<fig::string> text);

		bool Contains(const std::string& text) const noexcept;
	private:
		std::unordered_set<std::string> _words {};
	};
}
#endif