#include <pch.h>
#include "user/UserManager.h"

#include <random>
#include <chrono>
#include <filesystem>
#include <cassert>
#include <print>

#include "util/Hash.h"
#include "user/Security.h"
#include "io/Xml.h"
#include "io/AssetFileWriter.h"
#include "io/AssetFileReader.h"
#include "io/AssetManager.h"
#include "app/AppState.h"

using namespace fig::io;
using namespace fig::auth;

namespace fig::user
{
	static AuthChallenge CreateAuthChallenge(Bit128 key, Bit128 salt, AuthKey encKey)
	{
		AuthChallenge challenge;
		std::memcpy(challenge.data() + ptrdiff_t(0), key.data(), key.size());
		for (size_t i = 0; i < 16uz; ++i)
			challenge[i + 16uz] = challenge[i] ^ salt[i];
		auto encrypted = Encrypt(challenge, encKey);
		std::memcpy(challenge.data(), encrypted.data.data(), challenge.size());
		return challenge; // rvo
	}
	
	static bool Verify(const fig::string& text)
	{
		std::set<fig::string> used {};
		for (size_t i = 0; i < text.size() / 2; ++i)
		{
			fig::string bit = text.substr(i * 2, 2);
			used.insert(bit);
		}
		return used.size() == 256uz;
	}

	static void GenerateLUT()
	{
		static constexpr fig::const_string Symbols { "23456789ABCDEFGHJKLMNPQRTUVXW" }; // 0:O, 1:I, 2:Z, 5:S
		static std::mt19937_64 rng { std::default_random_engine{}() };
		static std::uniform_int_distribution<size_t> dist(0, Symbols.size() - 1);

		fig::string lut;
		lut.reserve(512);

		std::set<fig::string> used {};
		char tmp[3] { 'x', 'x', '\0' };
		for (size_t i = 0; i < 256; ++i)
		{
			while (true)
			{
				tmp[0] = Symbols[dist(rng)];
				tmp[1] = Symbols[dist(rng)];
				if (used.contains(tmp))
					continue;
				break;
			}
			used.insert(tmp);
			lut.append(tmp);
		}

		if (Verify(lut))
			std::println("{}", lut);
	}

	UserManager::UserManager()
	{
	}

	UserManager::~UserManager()
	{
		SignOut();
	}

	fig::optional_cref<UserProfile> UserManager::CreateDefaultProfile()
	{
		return CreateProfile(fig::string(fig::strings::UserProfile::DefaultUser), "");
	}

	fig::optional_cref<UserProfile> UserManager::CreateProfile(const fig::string& name, const fig::string& password)
	{
		bool hasPassword = not password.empty();
		auto authKey = RandomKey();
		auto authSalt = hasPassword ? RandomSalt() : CurrentDefaultAuthSalt;
		auto encKey = hasPassword ? DeriveKeyFromPassword(password, authSalt) : CurrentDefaultAuthKey;

		auto id = GenerateUUID();
		auto& db = GetDatabase();

		// Create auth challenge
		AuthChallenge authChallenge = CreateAuthChallenge(authKey, authSalt, encKey);

		UserProfile profile {
			.version { CurrentAuthVersion },
			.id { id },
			.name { name },
			.auth {
				.challenge = authChallenge,
				.salt = authSalt,
			},
			.has_password { hasPassword },
		};

		// Generate recovery key
		AuthKey recoveryKey;
		AuthChallenge recoveryChallenge;
		if (CreateRecoveryFile(profile, password, recoveryChallenge, recoveryKey))
		{
			profile.recovery = UserAuth {
				.challenge = recoveryChallenge,
				.salt = authSalt,
			};

			auto code = RecoveryKeyToCode(recoveryKey);
			LogLn(std::format("Recovery code for user {}: {}", profile.id.to_str(), code));
		}

		if (db.CreateProfile(profile) == DatabaseError::NoError)
		{
			_profiles.emplace_back(std::move(profile));
			return make_optional_cref(_profiles.back());
		}

		return fig::nullref;
	}

	static bool __Authenticate(const AuthChallenge& challenge, const AuthSalt& salt, const AuthKey& key, AuthKey& outKey)
	{
		auto decrypted = Decrypt(challenge, key);

		// Validate
		if (decrypted.size() != sizeof(AuthChallenge))
			return false;

		for (size_t i = 0; i < 16; ++i)
		{
			if (decrypted[i + 16uz] != (decrypted[i] ^ salt[i]))
				return false; // Invalid password
		}

		std::memcpy(outKey.data(), decrypted.data(), outKey.size());
		return true;
	}

	bool UserManager::Authenticate(const UserProfile& profile, const fig::string& password, AuthKey& outKey)
	{
		if (IsValidAuthVersion(profile.version))
		{
			auto encKey = not password.empty() ? DeriveKeyFromPassword(password, profile.auth.salt, profile.version) : GetAuthSettings(profile.version).DefaultAuthKey;
			return __Authenticate(profile.auth.challenge, profile.auth.salt, encKey, outKey);
		}
		return false;
	}

	bool UserManager::Authenticate(const AuthChallenge& challenge, const AuthSalt& salt, const AuthKey& key, AuthKey& outKey, AuthVersion version)
	{
		if (IsValidAuthVersion(version))
		{
			auto encKey = DeriveKeyFromBytes(key, salt, version);
			return __Authenticate(challenge, salt, encKey, outKey);
		}
		return false;
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
		bool success;
		DEBUG_MEASURE_BEGIN("SignIn");
		success = Authenticate(profile, password, _signedInAuthKey);
		DEBUG_MEASURE_END();

		if (not success)
			return false; // Incorrect password

		_signedInProfile = &profile;
		_pUserSettings = std::make_unique<UserSettings>(profile.GetPath() / Constants::Paths::UserSettings);
		_pUserSettings->Load();
		_pContentManager = std::make_unique<UserContentManager>(profile, _signedInAuthKey);
		return true;
	}

	bool UserManager::SignInDefaultProfile()
	{
		if (_profiles.empty())
			return false;
		if (auto lastProfile = GetProfile(Global::GetSettings().GetUUID(AppSetting::LastUser)); lastProfile.has_value() and not (*lastProfile).has_password)
			return SignIn(lastProfile.value(), "");
		return SignIn(_profiles.front(), "");
	}

	bool UserManager::SignOut()
	{
		if (not IsSignedIn())
			return false;

		_pContentManager.reset();

		_pUserSettings->Save();

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

		AuthKey authKey;
		if (not Authenticate(profile, oldPassword, authKey))
			return false;

		AuthSalt newSalt = not newPassword.empty() ? RandomSalt() : CurrentDefaultAuthSalt;
		AuthKey newPasswordKey = not newPassword.empty() ? DeriveKeyFromPassword(newPassword, newSalt) : CurrentDefaultAuthKey;
		AuthChallenge newChallenge = CreateAuthChallenge(authKey, newSalt, newPasswordKey);

		auto& db = GetDatabase();
		if (db.UpdateProfile(UserProfile {
			.version { CurrentAuthVersion },
			.id { profile.id },
			.name { profile.name },
			.auth {
				.challenge { newChallenge },
				.salt { newSalt },
			},
			.recovery = {},
			.has_password { !newPassword.empty() },
		}) == DatabaseError::NoError)
		{
			// Update local profile
			profile.version = CurrentAuthVersion;
			profile.auth = UserAuth {
				.challenge = newChallenge,
				.salt = newSalt,
			};
			profile.has_password = not newPassword.empty();
			return true;
		}
		return false;
	}

	fig::optional_cref<UserProfile> UserManager::GetActiveProfile() const noexcept
	{
		if (_signedInProfile == nullptr)
			return make_optional_cref(*_signedInProfile);
		return fig::nullref;
	}

	fig::optional_ref<UserProfile> UserManager::GetProfile(const fig::uuid& id) noexcept
	{
		if (auto itProfile = std::find_if(_profiles.begin(), _profiles.end(), [&id](const UserProfile& profile) { return profile.id == id; }); itProfile != _profiles.end())
			return make_optional_ref(*itProfile);
		return fig::nullref;
	}

	fig::optional_cref<UserProfile> UserManager::GetProfile(const fig::uuid& id) const noexcept
	{
		if (auto itProfile = std::find_if(_profiles.cbegin(), _profiles.cend(), [&id](const UserProfile& profile) { return profile.id == id; }); itProfile != _profiles.end())
			return make_optional_cref(*itProfile);
		return fig::nullref;
	}

	UserContentManager& UserManager::GetContent()
	{
		if (_signedInProfile == nullptr or !_pContentManager)
			throw std::runtime_error("Not signed in");

		return static_cast<UserContentManager&>(*_pContentManager);
	}

	UserSettings& UserManager::GetSettings()
	{
		if (_signedInProfile == nullptr or !_pUserSettings)
			throw std::runtime_error("Not signed in");

		return static_cast<UserSettings&>(*_pUserSettings);
	}

	fig::io::ProfileDatabase& UserManager::GetDatabase() noexcept
	{
		if (!_pProfileDB)
			_pProfileDB = std::make_unique<fig::io::ProfileDatabase>(fig::path(std::format("{}/{}.{}", Constants::Paths::ProfilesFolder, Constants::Paths::ProfilesFileName, Constants::Paths::ProfilesFileExt)));
		return *_pProfileDB.get();
	}

	bool UserManager::CreateRecoveryFile(const UserProfile& profile, const fig::string& password, AuthChallenge& recoveryChallenge, AuthKey& recoveryKey)
	{
		AuthKey authKey;
		if (Authenticate(profile, password, authKey) == false)
			return false;

		auto& salt = profile.auth.salt;
		recoveryKey = RandomKey();
	
		AuthKey encKey = DeriveKeyFromBytes(recoveryKey, salt);
		recoveryChallenge = CreateAuthChallenge(authKey, salt, encKey);
		
		return AssetFileWriter::WriteRecoveryFile(profile, recoveryChallenge) == FileError::NoError;
	}

	bool UserManager::RecoverProfile(const fig::uuid& profileID, const fig::string& recoveryCode)
	{
		AuthKey recoveryKey;
		if (RecoveryCodeToKey(recoveryCode, recoveryKey))
			return RecoverProfile(profileID, recoveryKey);
		return false;
	}

	bool UserManager::RecoverProfile(const fig::uuid& profileID, const AuthKey& recoveryKey)
	{
		auto itProfile = std::find_if(_profiles.begin(), _profiles.end(), [&profileID](const UserProfile& profile) {
			return profile.id == profileID;
		});

		if (itProfile == _profiles.cend())
			return false; // Not found

		auto& profile = *itProfile;
		if (is_zero(profile.recovery.challenge))
			return false;

		// Recover key
		AuthKey authKey;
		if (Authenticate(profile.recovery.challenge, profile.recovery.salt, recoveryKey, authKey))
		{
			// Reset password
			auto newChallenge = CreateAuthChallenge(authKey, CurrentDefaultAuthSalt, CurrentDefaultAuthKey);

			auto& db = GetDatabase();
			if (db.UpdateProfile(UserProfile {
				.version { CurrentAuthVersion },
				.id { profile.id },
				.name { profile.name },
				.auth {
					.challenge { newChallenge },
					.salt { CurrentDefaultAuthSalt },
				},
				.recovery {},
				.has_password {},
			}) == DatabaseError::NoError)
			{
				// Update local profile
				profile.version = CurrentAuthVersion;
				profile.auth.challenge = newChallenge;
				profile.auth.salt = CurrentDefaultAuthSalt;
				return true;
			}
			return false;
		}
		return false;
	}

	fig::string UserManager::RecoveryKeyToCode(const AuthKey& key, AuthVersion version) noexcept
	{
		fig::string code;
		code.reserve(35);

		auto& authSettings = GetAuthSettings(version);
		auto& recoveryLUT = authSettings.RecoveryKeyLUT;

		char tmp[2] { 'x', 'x' };
		const uint8_t* pKey = reinterpret_cast<const uint8_t*>(key.data());
		for (size_t i = 0; i < key.size(); ++i)
		{
			if (i > 0 and i % 4 == 0)
				code.append(" ");

			auto& lut = recoveryLUT[i % recoveryLUT.size()];
			assert(lut.size() == 512);
			std::memcpy(tmp, lut.data() + ptrdiff_t((uint16_t)key[i] * 2), 2uz);
			code.append(tmp, 2uz);
		}
		return code;
	}

	bool UserManager::RecoveryCodeToKey(const fig::string& code, AuthKey& outKey, AuthVersion version) noexcept
	{
		fig::string formatted = code
			| std::views::filter([](auto& c) { return std::isalnum((int)c); })
			| std::views::transform([](auto ch) { 
				ch = (char)std::toupper((int)ch);
				switch (ch)
				{
				case 'S': return '5';
				case 'Z': return '2';
				default: return ch;
				}
			})
			| std::ranges::to<fig::string>();

		if (formatted.size() != sizeof(AuthKey) * 2)
			return false;

		auto& authSettings = GetAuthSettings(version);
		auto& recoveryLUT = authSettings.RecoveryKeyLUT;

		static_assert(sizeof(AuthKey) == 16uz);
		for (size_t i = 0; i < 16uz; ++i)
		{
			char ch[2] { formatted[i * 2 + 0], formatted[i * 2 + 1] };
			auto& lut = recoveryLUT[i % recoveryLUT.size()];
			assert(lut.size() == 512);
			uint16_t n = 0;
			while (n < 512 and not (lut[n + 0] == ch[0] and lut[n + 1] == ch[1]))
				n += 2;
			if (n == 512)
				return false; // Error
			outKey[i] = static_cast<std::byte>(n / 2);
		}
		return true;
	}

	fig::uuid UserManager::GenerateUUID() const noexcept
	{
		fig::uuid uuid = _CreateUUID();
		while (std::ranges::contains(_profiles, uuid, [](auto& profile) { return profile.id; }))
			uuid = _CreateUUID();
		return uuid;
	}
}