#include <pch.h>
#include "model/UserManager.h"

#include <random>
#include <chrono>
#include <filesystem>

#include "util/Common.h"
#include "util/Hash.h"
#include "util/Security.h"
#include "util/ProfileDatabase.h"

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

	static fig::bytes CreateAuthChallenge(Bit128 key, Bit128 salt, AuthKey encKey)
	{
		fig::bytes challenge(32);
		std::memcpy(challenge.data() + ptrdiff_t(0), key.data(), key.size());
		for (size_t i = 0; i < 16uz; ++i)
			challenge[i + 16uz] = challenge[i] ^ salt[i];
		fig::security::Encrypt(challenge, encKey);
		return challenge; // rvo
	}

	std::optional<UserProfileCRef> UserManager::CreateDefaultProfile()
	{
		return CreateProfile(fig::string(fig::strings::UserProfile::DefaultUser), "");
	}

	std::optional<UserProfileCRef> UserManager::CreateProfile(const fig::string& name, const fig::string& password)
	{
		auto authKey = fig::security::Random128Bits();
		auto authSalt = not password.empty() ? fig::security::Random128Bits() : kDefaultAuthSalt;
		auto encKey = not password.empty() ? fig::security::DeriveKeyFromPassword(password, authSalt) : kDefaultAuthKey;

		auto id = common_util::CreateUUID();
		auto& db = GetDatabase();

		// Create auth challenge
		fig::bytes authChallenge = CreateAuthChallenge(authKey, authSalt, encKey);

		UserProfile profile {
			.id = id,
			.name = name,
			.authChallenge = authChallenge,
			.authSalt = authSalt,
		};

		if (db.CreateProfile(profile) == DatabaseError::NoError)
		{
			_profiles.emplace_back(profile);
			return std::make_optional(std::cref(_profiles.back()));
		}

		return std::nullopt;
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
		auto& db = GetDatabase();
		if (auto profiles = db.FetchProfiles(); profiles.has_value())
		{
			_profiles = std::move(profiles.value());
			return not _profiles.empty();
		}
		else
			return false;
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

		auto newChallenge = CreateAuthChallenge(authKey, profile.authSalt, newPasswordKey);

		auto& db = GetDatabase();
		if (db.UpdateProfile(UserProfile {
			.id = profile.id,
			.name = profile.name,
			.authChallenge = newChallenge,
			.authSalt = profile.authSalt,
		}) == DatabaseError::NoError)
		{
			// Update challenge
			profile.authChallenge = newChallenge;
			return true;
		}
		return false;
	}

	const UserProfile& UserManager::GetActiveProfile() const
	{
		if (_signedInProfile == nullptr)
			throw std::runtime_error("Not signed in");

		return std::cref(*_signedInProfile);
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

	fig::user::ProfileDatabase& UserManager::GetDatabase() noexcept
	{
		if (!_pProfileDB)
			_pProfileDB = std::make_unique<fig::user::ProfileDatabase>(fig::path(std::format("{}/{}.{}", Constants::Paths::ProfilesFolder, Constants::Paths::ProfilesFileName, Constants::Paths::ProfilesFileExt)));
		return *_pProfileDB.get();
	}
}