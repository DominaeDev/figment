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
	fig::hash GetHash(const fig::string& text)
	{
		fig::hash hash {};
		static_assert(sizeof(hash.parts) == 32uz);

		SHA256_CTX ctx;
		sha256_init(&ctx);
		sha256_update(&ctx, (SHA256_BYTE*)text.data(), text.size());
		sha256_final(&ctx, (SHA256_BYTE*)&hash.parts);

		for (auto& part : hash.parts)
			part = std::byteswap(part);

		return hash;
	}

	fig::hash GetHash(fig::byte_span data)
	{
		fig::hash hash {};
		static_assert(sizeof(hash.parts) == 32uz);

		SHA256_CTX ctx;
		sha256_init(&ctx);
		sha256_update(&ctx, (SHA256_BYTE*)data.data(), data.size());
		sha256_final(&ctx, (SHA256_BYTE*)&hash.parts);

		for (auto& part : hash.parts)
			part = std::byteswap(part);
		return hash;
	}

	fig::hash GetHash(const fig::path& filename)
	{
		std::ifstream file(filename, std::ios::binary | std::ios::in);
		if (!file)
			return {};

		SHA256_CTX ctx;
		sha256_init(&ctx);
		std::vector<uint8_t> buffer(1024 * 1024); // 1MB

		while (file.read(reinterpret_cast<char*>(buffer.data()), buffer.size()) || file.gcount() > 0)
			sha256_update(&ctx, buffer.data(), static_cast<size_t>(file.gcount()));

		if (file.bad())
			return {};

		fig::hash hash {};
		sha256_final(&ctx, (SHA256_BYTE*)&hash.parts);
		static_assert(sizeof(hash.parts) == 32uz);

		for (auto& part : hash.parts)
			part = std::byteswap(part);
		return hash;
	}

	fig::hash HashCombine(fig::hash a, fig::hash b, size_t& seed)
	{
		fig::hash c { a };
		for (size_t i = 0; i < 8; ++i)
			seed = c.parts[i] ^= b.parts[i] + 0x9e3779b9U + (seed << 6) + (seed >> 2);
		return c;
	}
}