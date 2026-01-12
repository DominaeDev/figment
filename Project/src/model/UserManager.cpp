#include <pch.h>
#include <random>
#include <chrono>
#include "util/Common.h"
#include "util/Hash.h"
#include "util/Encrypt.h"
#include "model/UserManager.h"

using namespace fig::encrypt;

namespace fig::fs
{
	static fig::const_string DefaultPassword { "||NO PASSWORD PROTECTION||" };
	constexpr std::array<fig::byte, 29> Challenge { 0x3c_byte, 0x41_byte, 0x55_byte, 0x54_byte, 0x48_byte, 0x3e_byte, 0x00_byte, 0x00_byte, 0x00_byte, 0x00_byte, 0x00_byte, 0x00_byte, 0x00_byte, 0x00_byte, 0x00_byte, 0x00_byte, 0x00_byte, 0x00_byte, 0x00_byte, 0x00_byte, 0x00_byte, 0x00_byte, 0x3c_byte, 0x2f_byte, 0x41_byte, 0x55_byte, 0x54_byte, 0x48_byte, 0x3e_byte, };
	constexpr size_t ChallengeAuthPosition = 6uz;

	static Bit128 Random128Bits()
	{
		static_assert(sizeof(Bit128) == 16);
		static std::mt19937_64 rng { std::random_device{}() };
		constexpr size_t u64_size = sizeof(uint64_t);
		Bit128 key;
		for (size_t n = 0; n < 16; n += u64_size)
		{
			auto r = rng();
			std::memcpy(&key[n], &r, u64_size);
		}
		return key;
	}

	static Key AuthKeyFromPassword(string password, Bit128 salt)
	{
		static_assert(sizeof(fig::Hash) == sizeof(Key));

		size_t seed;
		fig::Hash hash = common_util::GetHash(password);
		hash = common_util::HashCombine(hash, common_util::GetHash(salt), seed);

		Key authKey;
		std::memcpy(authKey.data(), &hash, sizeof(Key));
		return authKey;
	}

	UserProfile& UserManager::CreateProfile(fig::string name, fig::string password)
	{
		if (password.empty())
			password = DefaultPassword;

		auto authKey = Random128Bits();
		auto salt = Random128Bits();
		auto encKey = AuthKeyFromPassword(password, salt);

		// Create decryption challenge
		fig::bytes challenge;
		challenge.resize(Challenge.size());
		std::memcpy(challenge.data(), Challenge.data(), Challenge.size());
		std::memcpy(challenge.data() + ChallengeAuthPosition, authKey.data(), authKey.size());

		// Encrypt challenge
		common_util::Encrypt(challenge, encKey);
//		common_util::Decrypt(challenge, encKey);

		_profiles.emplace_back(UserProfile {
			.id = common_util::CreateUUID(),
			.name = name,
			.authKey {}, // Not stored
			.authSalt = salt,
			.authChallenge = challenge,
		});
		return _profiles.back();
	}
}