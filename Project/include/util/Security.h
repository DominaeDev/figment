#ifndef SECURITY_H__
#define SECURITY_H__
#pragma once

#include "Types.h"
#include <fstream>

namespace fig::security
{
	using Bit128 = std::array<fig::byte, 16uz>;
	using Bit256 = std::array<fig::byte, 32uz>;
	using AuthKey = Bit128;
	using AuthSalt = Bit128;

	using DecryptedData = fig::bytes;
	struct EncryptedData
	{
		fig::bytes data;
		size_t original_size;
		constexpr size_t encrypted_size() const noexcept { return data.size(); }
	};

	void Encrypt(fig::bytes& data, const AuthKey& key);
	void Encrypt(std::ofstream& stream, fig::byte_span in_data, const AuthKey& key);
	EncryptedData Encrypt(const fig::bytes& input, const AuthKey& key);
	EncryptedData Encrypt(fig::bytes&& input, const AuthKey& key);

	void Decrypt(fig::bytes& data, const AuthKey& key);
	void Decrypt(std::ifstream& stream, fig::bytes& out_data, const AuthKey& key);
	DecryptedData Decrypt(const fig::security::EncryptedData& input, const AuthKey& key);
	DecryptedData Decrypt(fig::security::EncryptedData&& input, const AuthKey& key);

	AuthKey DeriveKeyFromPassword(const fig::string& password, const fig::security::AuthSalt& salt);

	Bit128 Random128Bits();
	Bit256 Random256Bits();
}

#endif
