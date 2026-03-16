#ifndef SEARCH_INDEX_H__
#define SEARCH_INDEX_H__
#pragma once

#include "Types.h"

namespace fig
{
	class SearchIndex
	{
	public:
		void AddTerm(const fig::string& text) noexcept;
		void AddTerms(std::span<const fig::string> texts) noexcept;

		bool Match(const fig::string& search_string) const noexcept;
		inline bool IsEmpty() const noexcept { return _index.empty(); }

		fig::string Serialize() const noexcept;
		size_t Deserialize(const fig::string& data) noexcept;

	private:
		std::vector<fig::wstring> _index {};
	};
}
#endif
