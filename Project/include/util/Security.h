#ifndef SECURITY_H__
#define SECURITY_H__
#pragma once

#include "Types.h"
#include <fstream>

namespace fig::security
{
	using Bit128 = std::array<fig::byte, 16uz>;
	using Bit256 = std::array<fig::byte, 32uz>;
	using AuthKey = Bit256;
	using AuthSalt = Bit256;
	using AESKey = Bit256;

	using DecryptedData = fig::bytes;
	struct EncryptedData
	{
		fig::bytes data;
		size_t original_size;
		constexpr size_t encrypted_size() const noexcept { return data.size(); }
	};

	void Encrypt(std::ofstream& stream, fig::byte_span in_data, const fig::security::AESKey& key);
	EncryptedData Encrypt(const fig::bytes& input, const fig::security::AESKey& key);
	EncryptedData Encrypt(fig::bytes&& input, const fig::security::AESKey& key);

	void Decrypt(std::ifstream& stream, fig::bytes& out_data, const fig::security::AESKey& key);
	DecryptedData Decrypt(const fig::security::EncryptedData& input, const fig::security::AESKey& key);
	DecryptedData Decrypt(fig::security::EncryptedData&& input, const fig::security::AESKey& key);

	AESKey DeriveKeyFromPassword(const fig::string& password, const fig::security::AuthSalt& salt);

	Bit128 Random128Bits();
	Bit256 Random256Bits();
}

#endif
