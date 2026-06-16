#include <pch.h>
#include "util/Hash.h"

namespace fig
{
	Hash Hash::Empty = {};

	Hash::Hash(const Hash& other)
	{
		std::memcpy(&parts, &other.parts, sizeof(parts));
	}

	Hash& Hash::operator=(const Hash& other)
	{
		std::memcpy(&parts, &other.parts, sizeof(parts));
		return *this;
	}
}

extern "C" {
#include <sha256.h>
}

namespace fig
{
	fig::Hash GetHash(const fig::string& text)
	{
		fig::Hash hash {};
		static_assert(sizeof(hash.parts) == SHA256_BYTES_SIZE);
		sha256_bytes(text.data(), text.size(), &hash.parts);
		return hash;
	}

	fig::Hash GetHash(fig::byte_span data)
	{
		Hash hash {};
		static_assert(sizeof(hash.parts) == SHA256_BYTES_SIZE);
		sha256_bytes(data.data(), data.size(), &hash.parts);
		return hash;
	}

	fig::Hash HashCombine(fig::Hash a, fig::Hash b, size_t& seed)
	{
		fig::Hash c { a };
		for (size_t i = 0; i < 8; ++i)
			seed = c.parts[i] ^= b.parts[i] + 0x9e3779b9U + (seed << 6) + (seed >> 2);
		return c;
	}
}