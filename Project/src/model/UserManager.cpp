#include <pch.h>
#include "model/UserManager.h"

#include <random>
#include <chrono>
#include <filesystem>

#include "util/Common.h"
#include "util/Hash.h"
#include "util/Security.h"
#include "fs/Xml.h"
#include "model/AssetManager.h"
#include "model/GlobalStrings.h"

using namespace fig::security;

namespace fig::fs
{
	static fig::security::Bit128 kDefaultAuthKey { 
		fig::byte { 0xA1 }, fig::byte { 0xB2 }, fig::byte { 0xC3 }, fig::byte { 0xD4 }, fig::byte { 0xE5 }, fig::byte { 0xF6 }, fig::byte { 0xAB }, fig::byte { 0xCD },
		fig::byte { 0xEF }, fig::byte { 0x1A }, fig::byte { 0x2B }, fig::byte { 0x3C }, fig::byte { 0x4D }, fig::byte { 0x5E }, fig::byte { 0x6F }, fig::byte { 0xF0 },
	};
	static fig::security::Bit128 kDefaultAuthSalt { 
		fig::byte { 0xEF }, fig::byte { 0x1A }, fig::byte { 0x2B }, fig::byte { 0x3C }, fig::byte { 0x4D }, fig::byte { 0x5E }, fig::byte { 0x6F }, fig::byte { 0xF0 },
		fig::byte { 0xA1 }, fig::byte { 0xB2 }, fig::byte { 0xC3 }, fig::byte { 0xD4 }, fig::byte { 0xE5 }, fig::byte { 0xF6 }, fig::byte { 0xAB }, fig::byte { 0xCD },
	};

	UserProfile& UserManager::CreateDefaultProfile()
	{
		return CreateProfile(fig::string(fig::strings::UserProfile::DefaultUser), "");
	}

	static fig::bytes CreateChallenge(Bit128 key, Bit128 salt, AuthKey encKey)
	{
		fig::bytes challenge(32);
		std::memcpy(challenge.data() + ptrdiff_t(0), key.data(), key.size());
		for (size_t i = 0; i < 16uz; ++i)
			challenge[i + 16uz] = challenge[i] ^ salt[i];
		fig::security::Encrypt(challenge, encKey);
		return challenge; // rvo
	}

	UserProfile& UserManager::CreateProfile(const fig::string& name, const fig::string& password)
	{
		auto authKey = fig::security::Random128Bits();
		auto authSalt = not password.empty() ? fig::security::Random128Bits() : kDefaultAuthSalt;
		auto encKey = not password.empty() ? fig::security::DeriveKeyFromPassword(password, authSalt) : kDefaultAuthKey;

		// Create auth challenge
		fig::bytes authChallenge = CreateChallenge(authKey, authSalt, encKey);

		_profiles.emplace_back(UserProfile {
			.id = common_util::CreateUUID(),
			.name = name,
			.authChallenge = authChallenge,
			.authSalt = authSalt,
		});

		return _profiles.back();
	}

	bool UserManager::Authenticate(const UserProfile& profile, const fig::string& password, fig::security::AuthKey& outKey)
	{
		auto encKey = not password.empty() ? fig::security::DeriveKeyFromPassword(password, profile.authSalt) : kDefaultAuthKey;
		auto decrypted = fig::security::Decrypt(
			EncryptedData {
				.data = profile.authChallenge,
				.original_size = profile.authChallenge.size(),
			}, encKey);

		// Validate
		if (decrypted.size() != 32)
			return false;

		for (size_t i = 0; i < 16; ++i)
		{
			if (decrypted[i + 16uz] != (decrypted[i] ^ profile.authSalt[i]))
				return false; // Invalid password
		}
		
		std::memcpy(outKey.data(), decrypted.data(), outKey.size());
		return true;
	}

	bool UserManager::SignIn(const fig::string& profileName, const fig::string& password)
	{
		auto itProfile = std::find_if(_profiles.begin(), _profiles.end(), [&profileName](const UserProfile& profile) {
			return profile.name == profileName;
		});

		if (itProfile == _profiles.cend())
			return false; // Not found

		return SignIn(*itProfile, password);
	}

	bool UserManager::SignIn(const fig::uuid& profileID, const fig::string& password)
	{
		auto itProfile = std::find_if(_profiles.begin(), _profiles.end(), [&profileID](const UserProfile& profile) {
			return profile.id == profileID;
		});

		if (itProfile == _profiles.cend())
			return false; // Not found

		return SignIn(*itProfile, password);
	}

	bool UserManager::SignIn(UserProfile& profile, const fig::string& password)
	{
		if (not Authenticate(profile, password, _signedInAuthKey))
			return false; // Incorrect password

		_signedInProfile = &profile;
		_pAssetMngr = std::make_unique<AssetManager>(*this);
		_pContentDatabase = std::make_unique<ContentDatabase>(*_pAssetMngr.get());

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

		_pContentDatabase.reset();

		_pAssetMngr->SaveModified();
		_pAssetMngr.reset();

		_signedInProfile = nullptr;
		_signedInAuthKey = AuthKey {};
		return true;
	}

	bool UserManager::LoadProfiles()
	{
		fig::path path(std::format("{}/{}.{}", Constants::Paths::ProfilesFolder, Constants::Paths::ProfilesFileName, Constants::Paths::ProfilesFileExt));
		fig::XmlReader xml(path.u8string(), "Profiles");
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
			auto authData = profileNode.GetElementBytes("Auth").value_or({});
			if (authData.size() > sizeof(AuthSalt))
			{
				profile.authChallenge.resize(authData.size() - sizeof(AuthSalt));
				std::memcpy(profile.authSalt.data(), authData.data(), sizeof(AuthSalt));
				std::memcpy(profile.authChallenge.data(), authData.data() + sizeof(AuthSalt), profile.authChallenge.size());
			}

			if (profile.IsValid())
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

			profileNode.SetElementValue("ID", profile.id);
			profileNode.SetElementValue("Name", profile.name);

			fig::bytes auth(profile.authChallenge.size() + sizeof(AuthSalt));
			std::memcpy(auth.data(), profile.authSalt.data(), sizeof(AuthSalt));
			std::memcpy(auth.data() + sizeof(AuthSalt), profile.authChallenge.data(), profile.authChallenge.size());
			profileNode.SetElementValue("Auth", auth);
		}

		fig::path path(std::format("{}/{}.{}", Constants::Paths::ProfilesFolder, Constants::Paths::ProfilesFileName, Constants::Paths::ProfilesFileExt));
		return xml.Save(path.u8string());
	}

	bool UserManager::ChangePassword(const fig::uuid& profileID, const fig::string& oldPassword, const fig::string& newPassword)
	{
		auto itProfile = std::find_if(_profiles.begin(), _profiles.end(), [&profileID](const UserProfile& profile) {
			return profile.id == profileID;
		});

		if (itProfile == _profiles.cend())
			return false; // Not found

		auto& profile = *itProfile;

		fig::security::AuthKey authKey;
		if (not Authenticate(profile, oldPassword, authKey))
			return false;

		AuthKey newPasswordKey = not newPassword.empty() ? fig::security::DeriveKeyFromPassword(newPassword, profile.authSalt) : kDefaultAuthKey;

		// Update challenge
		profile.authChallenge = CreateChallenge(authKey, profile.authSalt, newPasswordKey);
		return true;
	}

	const UserProfile& UserManager::GetActiveProfile() const
	{
		if (_signedInProfile == nullptr)
			throw std::runtime_error("Not signed in");

		return static_cast<UserProfile&>(*_signedInProfile);
	}

	AssetManager& UserManager::GetProfileAssets()
	{
		if (_signedInProfile == nullptr || !_pAssetMngr)
			throw std::runtime_error("Not signed in");

		return static_cast<AssetManager&>(*_pAssetMngr);
	}

	ContentDatabase& UserManager::GetContent()
	{
		if (_signedInProfile == nullptr || !_pContentDatabase)
			throw std::runtime_error("Not signed in");

		return static_cast<ContentDatabase&>(*_pContentDatabase);
	}
}