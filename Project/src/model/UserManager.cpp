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
#include <tinyxml2.h>

using namespace fig::security;
using namespace tinyxml2;

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

	static fig::const_string kProfilesFilePath { "./profiles/index.xml" };

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

			profile.version = profileNode["version"].AsInt().value_or((unsigned short)-1);

			// ID
			fig::string id = profileNode.GetElementText("ID").value_or("");
			if (not id.empty())
				profile.id.fromStr(id.c_str()); //! @unsafe

			// Name
			profile.name = profileNode.GetElementText("Name").value_or("");

			// Auth challenge
			profile.authChallenge = profileNode.GetElementBytes("Auth").value_or({});

			// Auth challenge
			if (auto saltNode = profileNode.GetFirstElement("Salt"))
			{
				auto salt = saltNode.value().GetBytesText().value_or({});
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
		XMLDocument xmlDoc;
		auto pDecl = xmlDoc.NewDeclaration(nullptr);
		xmlDoc.InsertFirstChild(pDecl);

		auto pRoot = xmlDoc.NewElement("Profiles");
		xmlDoc.InsertEndChild(pRoot);

		for (auto& profile : _profiles)
		{
			auto pProfile = pRoot->InsertNewChildElement("Profile");
			pProfile->SetAttribute("version", toI(profile.version));

			// ID
			pProfile->InsertNewChildElement("ID")->InsertNewText(profile.id.str().c_str());

			// Name
			pProfile->InsertNewChildElement("Name")->InsertNewText(profile.name.c_str());

			// Auth challenge
			pProfile->InsertNewChildElement("Auth")->InsertNewText(common_util::Base64Encode(profile.authChallenge).c_str());

			// Auth challenge
			pProfile->InsertNewChildElement("Salt")->InsertNewText(common_util::Base64Encode(profile.authSalt).c_str());
		}

		auto const filename = std::filesystem::path(kProfilesFilePath);
		return xmlDoc.SaveFile(filename.generic_u8string().c_str()) == XML_SUCCESS;
	}
}