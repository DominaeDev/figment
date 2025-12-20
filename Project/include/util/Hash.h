#ifndef HASH_H__
#define HASH_H__
#pragma once

#include "Types.h"

namespace fig::common_util
{
	struct SHA_256 {
		uint32_t parts[8] {};

		fig::string to_string() const noexcept
		{
			return std::format("{:x}{:x}{:x}{:x}{:x}{:x}{:x}{:x}", parts[0], parts[1], parts[2], parts[3], parts[4], parts[5], parts[6], parts[7]);
		}

		explicit operator fig::string() const { return to_string(); }
		auto operator<=>(const SHA_256& rhs) const;

		static SHA_256 Empty;
	};

	using Hash = SHA_256;

	[[nodiscard]] Hash GetHash(const fig::string& text);
	[[nodiscard]] Hash GetHash(fig::byte_span data);
}
#endif