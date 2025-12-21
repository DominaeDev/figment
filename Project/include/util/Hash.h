#ifndef HASH_H__
#define HASH_H__
#pragma once

#include "Types.h"

namespace fig
{
	struct Hash 
	{
		uint32_t parts[8] = {0};

		Hash() = default;
		Hash(const Hash& other);
		Hash(Hash&& other) = default;
		Hash& operator=(const Hash& other);
		Hash& operator=(Hash&& other) = default;
		auto operator<=>(const Hash& rhs) const;

		fig::string to_string() const noexcept
		{
			return std::format("{:x}{:x}{:x}{:x}{:x}{:x}{:x}{:x}", parts[0], parts[1], parts[2], parts[3], parts[4], parts[5], parts[6], parts[7]);
		}

		explicit operator fig::string() const { return to_string(); }

		static Hash Empty;
	};
}

namespace fig::common_util
{
	[[nodiscard]] fig::Hash GetHash(const fig::string& text);
	[[nodiscard]] fig::Hash GetHash(fig::byte_span data);
	[[nodiscard]] fig::Hash HashCombine(fig::Hash a, fig::Hash b, size_t& seed);

	template <typename T, typename... Rest>
	void hash_combine(std::size_t& seed, const T& v, const Rest&... rest)
	{
		seed ^= std::hash<T>{}(v)+0x9e3779b9 + (seed << 6) + (seed >> 2);
		(hash_combine(seed, rest), ...);
	}
}
#endif