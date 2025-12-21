#include <pch.h>
#include "util/Hash.h"

extern "C" {
#include <sha256.h>
}

namespace fig::common_util
{
	SHA_256 SHA_256::Empty = {0};

	SHA_256 GetHash(const fig::string& text)
	{
		SHA_256 hash {};
		static_assert(sizeof(hash.parts) == SHA256_BYTES_SIZE);
		sha256_bytes(text.data(), text.size(), &hash.parts);
		return hash;
	}

	SHA_256 GetHash(fig::byte_span data)
	{
		SHA_256 hash {};
		static_assert(sizeof(hash.parts) == SHA256_BYTES_SIZE);
		sha256_bytes(data.data(), data.size(), &hash.parts);
		return hash;
	}

	auto SHA_256::operator<=>(const SHA_256& rhs) const
	{
		auto cmp = std::memcmp(&parts, &rhs.parts, sizeof(parts));
		return cmp == 0 ? std::strong_ordering::equal :
			cmp < 0 ? std::strong_ordering::less :
			std::strong_ordering::greater;
	}
}