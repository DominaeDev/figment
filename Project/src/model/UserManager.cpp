#include <pch.h>
#include "model/UserManager.h"

#include <random>
#include <chrono>
#include <filesystem>

#include "util/Common.h"
#include "util/Hash.h"
#include "util/Security.h"
#include "util/Xml.h"
#include "model/AssetManager.h"
#include "model/GlobalStrings.h"

using namespace fig::security;

namespace fig::fs
{
	static fig::const_string kDefaultPassword { "||NO PASSWORD PROTECTION||" };

	UserProfile& UserManager::CreateDefaultProfile()
	{
		return CreateProfile(fig::string(fig::strings::UserProfile::DefaultUser), fig::string(kDefaultPassword));
	}

	static fig::bytes CreateChallenge(Bit256 key, Bit256 salt, AuthKey encKey)
	{
		fig::bytes challenge(64);
		std::memcpy(challenge.data() + ptrdiff_t(0), key.data(), key.size());
		for (size_t i = 0; i < 32uz; ++i)
			challenge[i + 32u] = challenge[i] ^ salt[i];
		fig::security::Encrypt(challenge, encKey);
		return challenge; // rvo
	}

	UserProfile& UserManager::CreateProfile(const fig::string& name, const fig::string& password)
	{
		auto authKey = fig::security::Random256Bits();
		auto authSalt = fig::security::Random256Bits();
		auto encKey = fig::security::DeriveKeyFromPassword(not password.empty() ? password : fig::string(kDefaultPassword), authSalt);

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
		auto key = fig::security::DeriveKeyFromPassword(not password.empty() ? password : fig::string(kDefaultPassword), profile.authSalt);
		auto decrypted = fig::security::Decrypt(
			EncryptedData {
				.data = profile.authChallenge,
				.original_size = profile.authChallenge.size(),
			}, key);

		// Validate
		if (decrypted.size() != 64)
			return false;

		for (size_t i = 0; i < 32; ++i)
		{
			if (decrypted[i + 32uz] != (decrypted[i] ^ profile.authSalt[i]))
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

		_signedInProfile = nullptr;
		_signedInAuthKey = AuthKey {};
		_pAssetMngr.reset();
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

			// Auth challenge
			if (auto saltNode = profileNode.GetFirstElement("AuthSalt"))
			{
				auto salt = saltNode.value().GetBytes().value_or({});
				std::memcpy(profile.authSalt.data(), salt.data(), std::min(salt.size(), profile.authSalt.size()));
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

			profileNode.SetElement("ID", profile.id);
			profileNode.SetElement("Name", profile.name);

			fig::bytes auth(profile.authChallenge.size() + sizeof(AuthSalt));
			std::memcpy(auth.data(), profile.authSalt.data(), sizeof(AuthSalt));
			std::memcpy(auth.data() + sizeof(AuthSalt), profile.authChallenge.data(), profile.authChallenge.size());
			profileNode.SetElement("Auth", auth);
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

		auto newPasswordKey = fig::security::DeriveKeyFromPassword(not newPassword.empty() ? newPassword : fig::string(kDefaultPassword), profile.authSalt);

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
}