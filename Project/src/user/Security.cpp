#include <pch.h>
#include <cassert>
#include "user/Security.h"
#include "util/Hash.h"
#include "AES.h"
#include "io/FileStream.h"

namespace fig::auth
{

	static fig::bytes hmac_compute(std::span<const std::byte> key, std::span<const std::byte> data)
	{
		constexpr size_t block_size = 64;
		fig::bytes k(block_size, std::byte { 0 });

		if (key.size() > block_size)
		{
			auto hashed = GetHash(key).to_bytes();
			std::copy(hashed.begin(), hashed.end(), k.begin());
		}
		else
		{
			std::copy(key.begin(), key.end(), k.begin());
		}

		fig::bytes o_key_pad(block_size), i_key_pad(block_size);
		for (size_t i = 0; i < block_size; ++i)
		{
			o_key_pad[i] = k[i] ^ std::byte { 0x5c };
			i_key_pad[i] = k[i] ^ std::byte { 0x36 };
		}

		fig::bytes inner_data;
		inner_data.insert(inner_data.end(), i_key_pad.begin(), i_key_pad.end());
		inner_data.insert(inner_data.end(), data.begin(), data.end());
		auto inner_hash = GetHash(inner_data).to_bytes();

		fig::bytes outer_data;
		outer_data.insert(outer_data.end(), o_key_pad.begin(), o_key_pad.end());
		outer_data.insert(outer_data.end(), inner_hash.begin(), inner_hash.end());

		return GetHash(outer_data).to_bytes();
	}

	static fig::bytes PBKDF2(std::span<const std::byte> password, std::span<const std::byte> salt, uint32_t iterations, size_t output_length)
	{
		constexpr size_t hash_len = 32; // SHA-256 output length
		size_t blocks = (output_length + hash_len - 1) / hash_len;

		fig::bytes derived_key;
		derived_key.reserve(blocks * hash_len);

		for (uint32_t block = 1; block <= blocks; ++block)
		{
			// Prepare salt || INT(i)
			fig::bytes salt_block;
			salt_block.reserve(salt.size() + 4uz);
			salt_block.insert(salt_block.end(), salt.begin(), salt.end());
			salt_block.push_back(std::byte((block >> 24) & 0xff));
			salt_block.push_back(std::byte((block >> 16) & 0xff));
			salt_block.push_back(std::byte((block >> 8) & 0xff));
			salt_block.push_back(std::byte(block & 0xff));

			// U_1 = PRF(password, salt || INT(i))
			fig::bytes u = hmac_compute(password, salt_block);
			fig::bytes result { u };

			// U_j = PRF(password, U_{j-1})
			for (uint32_t j = 1; j < iterations; ++j)
			{
				u = hmac_compute(password, u);
				for (size_t k = 0; k < hash_len; ++k)
					result[k] ^= u[k];
			}

			derived_key.insert(derived_key.end(), result.begin(), result.end());
		}

		derived_key.resize(output_length);
		return derived_key;
	}

	static fig::bytes SimpleHash(fig::string password, fig::auth::AuthSalt salt)
	{
		size_t seed;
		fig::Hash hash = GetHash(password);
		hash = HashCombine(hash, GetHash(salt), seed);
		return hash.to_bytes();
	}

	static fig::bytes SimpleHash(fig::byte_span data, fig::auth::AuthSalt salt)
	{
		size_t seed;
		fig::Hash hash = GetHash(data);
		hash = HashCombine(hash, GetHash(salt), seed);
		return hash.to_bytes();
	}

	static void encrypt_data(unsigned char* pData, size_t length, const fig::auth::AuthKey& key)
	{
		static_assert(sizeof(unsigned char) == sizeof(std::byte));
		static_assert(sizeof(key) == 16);
		assert(length % 16 == 0);

		unsigned char u8Key[16];
		std::memcpy(u8Key, key.data(), key.size());
		Cipher::Aes<128> aes(u8Key);

		constexpr size_t stride = 16;
		for (size_t i = 0; i < length; i += stride)
			aes.encrypt_block(pData + i);
	}

	static void decrypt_data(unsigned char* pData, size_t length, const fig::auth::AuthKey& key)
	{
		static_assert(sizeof(unsigned char) == sizeof(std::byte));
		static_assert(sizeof(key) == 16);
		assert(length % 16 == 0);

		unsigned char u8Key[16];
		std::memcpy(u8Key, key.data(), key.size());
		Cipher::Aes<128> aes(u8Key);

		constexpr size_t stride = 16;
		for (size_t i = 0; i < length; i += stride)
			aes.decrypt_block(pData + i);
	}

	void Encrypt(std::ofstream& stream, fig::byte_span in_data, const fig::auth::AuthKey& key)
	{
		static_assert(sizeof(unsigned char) == sizeof(std::byte));
		static_assert(sizeof(key) == 16);

		unsigned char u8Key[16];
		std::memcpy(u8Key, key.data(), key.size());
		Cipher::Aes<128> aes(u8Key);

		constexpr size_t kBufferSize = 256;
		std::array<fig::byte, kBufferSize> buffer {};

		size_t length = in_data.size();
		for (size_t i = 0; i < length; i += kBufferSize)
		{
			std::memcpy(buffer.data(), in_data.data() + ptrdiff_t(i), std::min(buffer.size(), length - i));
			
			constexpr size_t stride = 16;
			for (size_t j = 0; j < buffer.size(); j += stride)
				aes.encrypt_block((unsigned char*)buffer.data() + ptrdiff_t(j));

			stream.write((const char*)buffer.data(), buffer.size());
		}
	}

	void Encrypt(fig::bytes& data, const AuthKey& key)
	{
		static_assert(sizeof(unsigned char) == sizeof(std::byte));
		static_assert(sizeof(key) == 16);
		assert(data.size() % 16 == 0);

		unsigned char u8Key[16];
		std::memcpy(u8Key, key.data(), key.size());
		Cipher::Aes<128> aes(u8Key);

		constexpr size_t stride = 16;
		for (size_t i = 0; i < data.size(); i += stride)
			aes.encrypt_block((unsigned char*)data.data() + ptrdiff_t(i));
	}

	EncryptedData Encrypt(const fig::bytes& input, const fig::auth::AuthKey& key)
	{
		auto size = input.size();
		EncryptedData encrypted {
			.original_size = input.size(),
		};

		auto padded_size = size % 16 == 0 ? size : (1 + size / 16) * 16;
		encrypted.data.resize(padded_size, 0_byte);
		std::memcpy(encrypted.data.data(), (void*)input.data(), input.size());

		encrypt_data(reinterpret_cast<unsigned char*>(encrypted.data.data()), encrypted.data.size(), key);
		return encrypted; // rvo
	}

	EncryptedData Encrypt(fig::bytes&& input, const fig::auth::AuthKey& key)
	{
		auto size = input.size();
		
		EncryptedData encrypted { .original_size = size };
		encrypted.data = std::move(input);
		auto padded_size = size % 16 == 0 ? size : (1 + size / 16) * 16;
		encrypted.data.resize(padded_size, 0_byte);

		auto encrypted_chars = bytes_to_u8(encrypted.data);
		encrypt_data(encrypted_chars.data(), encrypted_chars.size(), key);
		return encrypted; // rvo
	}

	EncryptedData Encrypt(const AuthChallenge& input, const fig::auth::AuthKey& key)
	{
		static_assert(sizeof(AuthChallenge) % 16 == 0);
		auto size = input.size();

		EncryptedData encrypted { .original_size = size };
		encrypted.data.resize(size, 0_byte);
		std::memcpy(encrypted.data.data(), input.data(), size);

		auto encrypted_chars = bytes_to_u8(encrypted.data);
		encrypt_data(encrypted_chars.data(), encrypted_chars.size(), key);
		return encrypted; // rvo
	}

	void Decrypt(std::ifstream& stream, fig::bytes& out_data, const fig::auth::AuthKey& key)
	{
		static_assert(sizeof(unsigned char) == sizeof(std::byte));
		static_assert(sizeof(key) == 16);

		unsigned char u8Key[16];
		std::memcpy(u8Key, key.data(), key.size());
		Cipher::Aes<128> aes(u8Key);

		constexpr size_t kBufferSize = 256;
		std::array<uint8_t, kBufferSize> buffer { 0 };

		size_t max_length = out_data.size();
		for (size_t i = 0; i < max_length; i += kBufferSize)
		{
			stream.read((char*)buffer.data(), kBufferSize);

			constexpr size_t stride = 16;
			for (size_t j = 0; j < kBufferSize; j += stride)
				aes.decrypt_block(buffer.data() + ptrdiff_t(j));

			std::memcpy(out_data.data() + ptrdiff_t(i), buffer.data(), std::min(kBufferSize, max_length - i));
		}
	}

	void Decrypt(fig::bytes& data, const AuthKey& key)
	{
		static_assert(sizeof(unsigned char) == sizeof(std::byte));
		static_assert(sizeof(key) == 16);
		assert(data.size() % 16 == 0);

		unsigned char u8Key[16];
		std::memcpy(u8Key, key.data(), key.size());
		Cipher::Aes<128> aes(u8Key);

		constexpr size_t stride = 16;
		for (size_t i = 0; i < data.size(); i += stride)
			aes.decrypt_block((unsigned char*)data.data() + ptrdiff_t(i));
	}

	DecryptedData Decrypt(const EncryptedData& input, const fig::auth::AuthKey& key)
	{
		assert(input.data.size() % 16 == 0);

		DecryptedData decrypted { input.data };
		decrypt_data(reinterpret_cast<unsigned char*>(decrypted.data()), decrypted.size(), key);
		decrypted.resize(input.original_size);
		return decrypted; // rvo
	}

	DecryptedData Decrypt(EncryptedData&& input, const fig::auth::AuthKey& key)
	{
		assert(input.data.size() % 16 == 0);

		DecryptedData decrypted { std::move(input.data) };
		auto decrypted_chars = bytes_to_u8(decrypted);
		decrypt_data(decrypted_chars.data(), decrypted_chars.size(), key);

		decrypted.resize(input.original_size);
		return decrypted; // rvo
	}

	DecryptedData Decrypt(const AuthChallenge& input, const fig::auth::AuthKey& key)
	{
		static_assert(sizeof(AuthChallenge) % 16 == 0);
		fig::bytes encData(input.size());
		std::memcpy(encData.data(), input.data(), input.size());

		return fig::auth::Decrypt(
			EncryptedData {
				.data { encData },
				.original_size { encData.size() },
			},
			key);

	}

	void Decrypt(fig::io::FileStream& fs, fig::bytes& out_data, const fig::auth::AuthKey& key)
	{
		static_assert(sizeof(unsigned char) == sizeof(std::byte));
		static_assert(sizeof(key) == 16);

		unsigned char u8Key[16];
		std::memcpy(u8Key, key.data(), key.size());
		Cipher::Aes<128> aes(u8Key);

		constexpr size_t kBufferSize = 256;
		fig::buffer<kBufferSize> buffer {};
		size_t max_length = out_data.size();
		for (size_t i = 0; i < max_length; i += kBufferSize)
		{
			size_t read = fs.Read(buffer);
			if (read == 0)
				break; // Error

			constexpr size_t stride = 16;
			for (size_t j = 0; j < kBufferSize; j += stride)
				aes.decrypt_block(reinterpret_cast<unsigned char*>(buffer.data()) + ptrdiff_t(j));
			std::memcpy(out_data.data() + ptrdiff_t(i), buffer.data(), std::min(kBufferSize, max_length - i));
		}
	}

	AuthKey DeriveKeyFromPassword(const fig::string& password, const fig::auth::AuthSalt& salt, AuthVersion version)
	{
		auto& authSettings = GetAuthSettings(version);
		
		fig::bytes derived_key;
		if (authSettings.KDF.Type == KDFType::PBKDF2)
		{
			auto password_bytes = string_to_bytes(password);
			auto salt_bytes = std::span { salt.data(), salt.size() };
			derived_key = PBKDF2(password_bytes, salt_bytes, authSettings.KDF.Iterations, sizeof(AuthKey));
		}
		else
		{
			derived_key = SimpleHash(password, salt);
		}

		AuthKey authKey;
		std::memcpy(authKey.data(), derived_key.data(), std::min(sizeof(AuthKey), derived_key.size()));
		return authKey;
	}

	AuthKey DeriveKeyFromBytes(fig::byte_span data, const fig::auth::AuthSalt& salt, AuthVersion version)
	{
		auto& authSettings = GetAuthSettings(version);
		
		fig::bytes derived_key;
		if (authSettings.KDF.Type == KDFType::PBKDF2)
		{
			auto password_bytes = data; // copy
			auto salt_bytes = std::span { salt.data(), salt.size() };
			derived_key = PBKDF2(password_bytes, salt_bytes, authSettings.KDF.Iterations, sizeof(AuthKey));
		}
		else
		{
			derived_key = SimpleHash(data, salt);
		}

		AuthKey authKey;
		std::memcpy(authKey.data(), derived_key.data(), std::min(sizeof(AuthKey), derived_key.size()));
		return authKey;
	}

	AuthKey RandomKey()
	{
		return AuthKey { Random128Bits() };
	}

	AuthSalt RandomSalt()
	{
		return AuthSalt { Random128Bits() };
	}

	Bit128 Random128Bits()
	{
		static std::mt19937_64 rng { std::random_device{}() };
		constexpr size_t u64_size = sizeof(uint64_t);
		Bit128 key;
		for (size_t n = 0; n < sizeof(Bit128); n += u64_size)
		{
			auto r = rng();
			std::memcpy(&key[n], &r, u64_size);
		}
		return key;
	}

	Bit256 Random256Bits()
	{
		static std::mt19937_64 rng { std::random_device{}() };
		constexpr size_t u64_size = sizeof(uint64_t);
		Bit256 key;
		for (size_t n = 0; n < sizeof(Bit256); n += u64_size)
		{
			auto r = rng();
			std::memcpy(&key[n], &r, u64_size);
		}
		return key;
	}

	const AuthSettings& GetAuthSettings(AuthVersion version)
	{
		assert(version < AuthSettingsByVersion.size());
		return AuthSettingsByVersion[version];
	}
}