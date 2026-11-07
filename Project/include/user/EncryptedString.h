#pragma once

#include "user/Security.h"

namespace fig::auth
{
	class EncryptedString
	{
		static constexpr size_t BufferSize { 256uz };
	public:
		EncryptedString() = default;
		explicit EncryptedString(fig::string_view text, AuthKey key);
		explicit EncryptedString(fig::byte_span data);

		EncryptedString(const EncryptedString& other) = default;
		EncryptedString(EncryptedString&& other) = default;
		EncryptedString& operator=(const EncryptedString& other) = default;
		EncryptedString& operator=(EncryptedString&& other) = default;

		std::optional<fig::string> Decrypt(AuthKey key) const;

		constexpr const fig::byte* const data() const noexcept { return _data; }
		constexpr size_t data_size() const noexcept { return BufferSize; }

	private:
		fig::byte _data[BufferSize] {};
	};
}