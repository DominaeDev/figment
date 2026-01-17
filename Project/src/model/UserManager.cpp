#include <pch.h>
#include <random>
#include <chrono>
#include "util/Common.h"
#include "util/Hash.h"
#include "util/Encrypt.h"
#include "model/UserManager.h"

using namespace fig::security;

namespace fig::fs
{
	static fig::const_string kDefaultProfileName { "Default profile" };
	static fig::const_string kDefaultPassword { "||NO PASSWORD PROTECTION||" };
	constexpr std::array<fig::byte, 48> kChallenge { 
		0x41_byte, 0x55_byte, 0x54_byte, 0x48_byte, 0x5f_byte, 0x4b_byte, 0x45_byte, 0x59_byte,
		0x00_byte, 0x00_byte, 0x00_byte, 0x00_byte, 0x00_byte, 0x00_byte, 0x00_byte, 0x00_byte, 
		0x00_byte, 0x00_byte, 0x00_byte, 0x00_byte, 0x00_byte, 0x00_byte, 0x00_byte, 0x00_byte, 
		0x00_byte, 0x00_byte, 0x00_byte, 0x00_byte, 0x00_byte, 0x00_byte, 0x00_byte, 0x00_byte, 
		0x00_byte, 0x00_byte, 0x00_byte, 0x00_byte, 0x00_byte, 0x00_byte, 0x00_byte, 0x00_byte,
		0x41_byte, 0x55_byte, 0x54_byte, 0x48_byte, 0x5f_byte, 0x4b_byte, 0x45_byte, 0x59_byte,
	};
	static fig::const_string kChallengeMagicWord { "AUTH_KEY" };
	constexpr size_t kChallengeKeyPosition = 8uz;

	UserProfile& UserManager::CreateDefaultProfile()
	{
		return CreateProfile(fig::string(kDefaultProfileName), fig::string(kDefaultPassword));
	}

	UserProfile& UserManager::CreateProfile(const fig::string& name, const fig::string& password)
	{
		auto authKey = fig::security::Random256Bits();
		auto authSalt = fig::security::Random256Bits();
		auto encKey = fig::security::DeriveKeyFromPassword(not password.empty() ? password : fig::string(kDefaultPassword), authSalt);

		// Create auth challenge
		fig::bytes challenge;
		challenge.resize(kChallenge.size());
		std::memcpy(challenge.data(), kChallenge.data(), kChallenge.size());
		std::memcpy(challenge.data() + kChallengeKeyPosition, authKey.data(), authKey.size());
		auto authChallenge = fig::security::Encrypt(std::move(challenge), encKey);

		_profiles.emplace_back(UserProfile {
			.id = common_util::CreateUUID(),
			.name = name,
			.authChallenge = authChallenge.data,
			.authSalt = authSalt,
		});

		return _profiles.back();
	}

	bool UserManager::SignIn(const fig::string& profileName, const fig::string& password)
	{
		auto itProfile = std::find_if(_profiles.cbegin(), _profiles.cend(), [&profileName](const UserProfile& profile) {
			return profile.name == profileName;
		});

		if (itProfile == _profiles.cend())
			return false; // Not found

		return SignIn(*itProfile, password);
	}

	bool UserManager::SignIn(const fig::uuid& profileID, const fig::string& password)
	{
		auto itProfile = std::find_if(_profiles.cbegin(), _profiles.cend(), [&profileID](const UserProfile& profile) {
			return profile.id == profileID;
		});

		if (itProfile == _profiles.cend())
			return false; // Not found

		return SignIn(*itProfile, password);
	}

	bool UserManager::SignIn(const UserProfile& profile, const fig::string& password)
	{
		auto encKey = fig::security::DeriveKeyFromPassword(not password.empty() ? password : fig::string(kDefaultPassword), profile.authSalt);

		EncryptedData encrypted {
			.data = profile.authChallenge,
			.original_size = kChallenge.size(),		
		};
		auto decryptedChallenge = fig::security::Decrypt(std::move(encrypted), encKey);

		// Validate
		auto prefix = std::strncmp((const char*)decryptedChallenge.data(), kChallengeMagicWord.data(), 8uz);
		auto suffix = std::strncmp((const char*)decryptedChallenge.data() + 40uz, kChallengeMagicWord.data(), 8uz);
		if (prefix or suffix)
			return false; // Incorrect password

		_signedInProfileId = profile.id;
		std::memcpy(_signedInAuthKey.data(), decryptedChallenge.data() + kChallengeKeyPosition, _signedInAuthKey.size());
		return true;
	}

	bool UserManager::SignOut()
	{
		if (not IsSignedIn())
			return false;

		_signedInProfileId = { 0 };
		_signedInAuthKey = AuthKey {};
		return true;
	}
}