#ifndef SEARCH_INDEX_H__
#define SEARCH_INDEX_H__
#pragma once

#include "Types.h"

namespace fig
{
	struct SearchQuery
	{
		explicit SearchQuery(const fig::string& query);
		std::vector<fig::string> terms;

		inline bool empty() const noexcept { return terms.empty(); };
	};

	class SearchIndex
	{
	public:
		void AddTerm(const fig::string& text) noexcept;
		void AddTerms(std::span<const fig::string> texts) noexcept;

		bool Match(const SearchQuery& query) const noexcept;
		inline bool IsEmpty() const noexcept { return _index.empty(); }

		fig::string Serialize() const noexcept;
		size_t Deserialize(const fig::string& data) noexcept;

	private:
		std::vector<fig::string> _index {};
	};
}
#endif
