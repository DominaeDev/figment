#ifndef HASH_H__
#define HASH_H__
#pragma once

#include "Figment.h"

namespace fig
{
	struct Hash 
	{
		std::array<uint32_t, 8> parts = {};
		static Hash Empty;

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

		inline fig::bytes to_bytes() const noexcept
		{
			std::vector<std::byte> result(32);
			for (size_t i = 0; i < 8; ++i)
			{
				result[i * 4 + 0uz] = fig::byte((parts[i] >> 24) & 0xff);
				result[i * 4 + 1uz] = fig::byte((parts[i] >> 16) & 0xff);
				result[i * 4 + 2uz] = fig::byte((parts[i] >> 8) & 0xff);
				result[i * 4 + 3uz] = fig::byte((parts[i] >> 0) & 0xff);
			}
			return result;
		}
	};
}

namespace fig::util
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