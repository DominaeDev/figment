#include <pch.h>
#include <random>
#include <chrono>
#include <filesystem>

#include "util/Common.h"
#include "util/Hash.h"
#include "util/Encrypt.h"
#include "util/Xml.h"
#include "model/UserManager.h"
#include "model/GlobalStrings.h"

using namespace fig::security;

namespace fig::fs
{
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

	static fig::const_string kProfilesFilePath { "./profiles/Profiles.xml" };

	UserProfile& UserManager::CreateDefaultProfile()
	{
		return CreateProfile(fig::string(fig::strings::UserProfile::DefaultUser), fig::string(kDefaultPassword));
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

	bool UserManager::SignInDefaultProfile()
	{
		return SignIn(fig::string(fig::strings::UserProfile::DefaultUser), "");
	}

	bool UserManager::SignOut()
	{
		if (not IsSignedIn())
			return false;

		_signedInProfileId = { 0 };
		_signedInAuthKey = AuthKey {};
		return true;
	}

	bool UserManager::LoadProfiles()
	{
		fig::XmlReader xml(toStr(kProfilesFilePath), "Profiles");
		if (not xml.IsOk())
			return false; // Invalid document type

		_profiles.clear();

		auto pProfile = xml.GetFirstElement("Profile");
		while (pProfile)
		{
			auto& profileNode = pProfile.value();
			UserProfile profile {};

			profile.version = profileNode["version"].AsInt((unsigned short)-1);

			// ID
			profile.id = profileNode.GetElementUUID("ID").value_or({});

			// Name
			profile.name = profileNode.GetElementText("Name", "");

			// Auth challenge
			profile.authChallenge = profileNode.GetElementBytes("AuthData").value_or({});

			// Auth challenge
			if (auto saltNode = profileNode.GetFirstElement("AuthSalt"))
			{
				auto salt = saltNode.value().GetBytes().value_or({});
				std::memcpy(profile.authSalt.data(), salt.data(), std::min(salt.size(), profile.authSalt.size()));
			}

			if (profile.is_valid())
				_profiles.push_back(profile);

			pProfile = profileNode.GetNextSibling();
		}
		
		return not _profiles.empty();
	}

	bool UserManager::SaveProfiles() const
	{
		XmlWriter xml("Profiles");
		for (auto& profile : _profiles)
		{
			auto profileNode = xml.AddChild("Profile");
			profileNode["version"] = toI(profile.version);

			profileNode.SetElement("ID", profile.id);
			profileNode.SetElement("Name", profile.name);
			profileNode.SetElement("AuthData", profile.authChallenge);
			profileNode.SetElement("AuthSalt", profile.authSalt);
		}

		return xml.Save(fig::string(kProfilesFilePath));
	}
}