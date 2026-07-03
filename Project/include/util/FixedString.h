#ifndef FIXED_STRING_H__
#define FIXED_STRING_H__
#pragma once

#include <array>

namespace fig
{
	template <size_t N>
	struct fixed_string 
	{
		constexpr fixed_string(const char(&str)[N])
		{
			for (size_t i = 0; i < N; ++i) 
				value[i] = str[i];
		}

		constexpr const char* c_str() const noexcept
		{
			return value.data();
		}

		std::array<char, N> value {};
	};
}

#endif
