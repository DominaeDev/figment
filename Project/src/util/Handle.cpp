#include <pch.h>
#include "util/Handle.h"
#include "util/StringUtils.h"

namespace fig
{
	std::string normalize_handle(std::string_view raw_handle, size_t max_length)
	{
		auto value = trim(std::string { raw_handle });
		std::string result;
		result.reserve(value.size());

		for (unsigned char c : value)
		{
			if (max_length > 0 and result.size() == max_length)
				break;

			if (std::isalnum(c))
				result += std::tolower(c);
			else if (c == '-')
				result += c;
			else if (c == ' ' or std::ispunct(c))
				result += '_';
			else if (std::isspace(c))
				continue;
			else
				result += '?';
		}

		return result;
	}

	std::wstring normalize_handle(std::wstring_view raw_handle, size_t max_length)
	{
		auto value = trim(std::wstring { raw_handle });

		std::wstring result;
		result.reserve(value.size());

		for (wint_t c : value)
		{
			if (max_length > 0 and result.size() == max_length)
				break;

			if (std::iswalnum(c))
				result += std::towlower(c);
			else if (c == L'-')
				result += c;
			else if (c == L' ' or std::iswpunct(c))
				result += L'_';
			else if (std::iswspace(c))
				continue;
			else
				result += L'?';
		}

		return result;
	}
}
