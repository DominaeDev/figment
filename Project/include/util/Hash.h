#pragma once

#include <array>
#include <stop_token>

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
		auto operator<=>(const Hash& other) const
		{
			auto cmp = std::memcmp(&parts, &other.parts, sizeof(parts));
			return cmp == 0 ? std::strong_ordering::equal :
				cmp < 0 ? std::strong_ordering::less :
				std::strong_ordering::greater;
		}
		bool operator==(const Hash& other) const = default;

		fig::string to_string() const noexcept
		{
			return std::format("{:08x}{:08x}{:08x}{:08x}{:08x}{:08x}{:08x}{:08x}", parts[0], parts[1], parts[2], parts[3], parts[4], parts[5], parts[6], parts[7]);
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

		bool empty() const noexcept
		{
			return (*this) == Hash::Empty;
		}
	};

	using hash = Hash;
}

namespace fig
{
	[[nodiscard]] fig::hash GetHash(const fig::string& text);
	[[nodiscard]] fig::hash GetHash(fig::byte_span data);
	[[nodiscard]] fig::hash GetHash(const fig::path& filename);
	[[nodiscard]] fig::hash GetHash(const fig::path& filename, std::stop_token stopToken);
	[[nodiscard]] fig::hash HashCombine(fig::hash a, fig::hash b, size_t& seed);

	template <typename T, typename... Rest>
	void hash_combine(std::size_t& seed, const T& v, const Rest&... rest)
	{
		seed ^= std::hash<T>{}(v)+0x9e3779b9 + (seed << 6) + (seed >> 2);
		(hash_combine(seed, rest), ...);
	}
}
