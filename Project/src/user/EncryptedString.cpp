#include <pch.h>
#include "user/EncryptedString.h"

namespace fig::auth
{
	EncryptedString::EncryptedString(fig::string_view text, AuthKey key)
	{
		static_assert(BufferSize % 16 == 0);
		constexpr auto MaxLength = BufferSize - 1uz;
		auto truncated = truncate_utf8(text, MaxLength);
		uint8_t length = static_cast<uint8_t>(truncated.size());

		fig::bytes buffer;
		buffer.resize(BufferSize, 0_byte);
		buffer[0] = std::byte { length };
		std::memcpy(&buffer[1], truncated.data(), std::min(truncated.size(), MaxLength));

		fig::auth::Encrypt(buffer, key);

		std::memcpy(_data, buffer.data(), BufferSize);
	}

	EncryptedString::EncryptedString(fig::byte_span data)
	{
		assert(data.size() == sizeof(_data));
		std::memcpy(_data, data.data(), std::min(data.size(), sizeof(_data)));
	}

	std::optional<fig::string> EncryptedString::Decrypt(AuthKey key) const
	{
		fig::bytes buffer(std::begin(_data), std::end(_data));
		fig::auth::Decrypt(buffer, key);

		size_t length = static_cast<size_t>(buffer[0]);
		for (size_t i = length + 1uz; i < BufferSize; ++i)
		{
			if (buffer[i] != 0_byte)
				return std::nullopt;
		}

		return fig::string { reinterpret_cast<char*>(&buffer[1]), length };
	}
}