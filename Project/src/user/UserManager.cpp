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
#include "io/FileUtility.h"
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
		return CreateProfile(fig::string(fig::strings::User::DefaultUser), "");
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
			.state { password.empty() ? UserProfile::State::Open : UserProfile::State::Locked }
		};

		SignOut();

		// Write to db
		if (db.CreateProfile(profile) == DatabaseError::NoError)
		{
			// Create path
			auto path = profile.GetPath();
			if (not std::filesystem::exists(path))
				std::filesystem::create_directory(path);

			// Write profile files
			if (SignIn(profile, password))
			{
				_pUserSettings->Save();
				_pContentManager->GetAssets().SaveNow();
				SignOut();
			}
			
			// Create recovery file
			AuthKey recoveryKey;
			AuthChallenge recoveryChallenge;
			if (CreateRecoveryFile(profile, authKey, recoveryChallenge, recoveryKey))
			{
				profile.recovery = {
					.challenge = recoveryChallenge,
					.salt = authSalt,
				};
				db.UpdateProfile(profile);
			}

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

	bool UserManager::SignIn(const fig::uuid& profileId, const fig::string& password)
	{
		auto itProfile = std::find_if(_profiles.begin(), _profiles.end(), [&profileId](const UserProfile& profile) {
			return profile.id == profileId;
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

		_pSignedInProfile = &profile;
		_pUserSettings = std::make_unique<UserSettings>(profile.GetPath() / Constants::Paths::UserSettings);
		_pUserSettings->Load();
		_pContentManager = std::make_unique<UserContentManager>(profile, _signedInAuthKey);
		return true;
	}

	bool UserManager::SignInDefaultProfile()
	{
		if (_profiles.empty())
			return false;
		if (auto lastProfile = GetProfile(Global::GetSettings().GetUUID(AppSetting::LastUser)); lastProfile.has_value() and (*lastProfile).state == UserProfile::State::Open)
			return SignIn(lastProfile.value(), "");
		return SignIn(_profiles.front(), "");
	}

	bool UserManager::SignOut()
	{
		if (not IsSignedIn())
			return false;

		_pContentManager.reset();

		_pUserSettings->Save();

		_pSignedInProfile = nullptr;
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

	bool UserManager::ChangePassword(const fig::uuid& profileId, const fig::string& oldPassword, const fig::string& newPassword)
	{
		auto itProfile = std::find_if(_profiles.begin(), _profiles.end(), [&profileId](const UserProfile& profile) {
			return profile.id == profileId;
		});

		if (itProfile == _profiles.cend())
			return false; // Not found

		auto& profile = *itProfile;

		AuthKey authKey;
		if (not Authenticate(profile, oldPassword, authKey))
			return false;

		if (not newPassword.empty())
		{
			AuthSalt newSalt = RandomSalt();
			AuthKey newPasswordKey = DeriveKeyFromPassword(newPassword, newSalt);
			AuthChallenge newChallenge = CreateAuthChallenge(authKey, newSalt, newPasswordKey);
			profile.auth = {
				.challenge { newChallenge },
				.salt { newSalt },
			};
			profile.state = UserProfile::State::Locked;
		}
		else
		{
			profile.auth = {
				.challenge { CreateAuthChallenge(authKey, CurrentDefaultAuthSalt, CurrentDefaultAuthKey) },
				.salt { CurrentDefaultAuthSalt },
			};
			profile.state = UserProfile::State::Open;
		}
		profile.version = CurrentAuthVersion;
		profile.recovery = {};

		if (GetDatabase().UpdateProfile(profile) == DatabaseError::NoError)
		{
			GenerateNewRecoveryKey();
			return true;
		}
		return false;
	}

	bool UserManager::ResetProfile(const fig::uuid& profileId)
	{
		auto itProfile = std::find_if(_profiles.begin(), _profiles.end(), [&profileId](const UserProfile& profile) {
			return profile.id == profileId;
		});

		if (itProfile == _profiles.cend())
			return false; // Not found
		
		auto authKey = RandomKey();
		auto& profile = *itProfile;
		profile.auth = {
			.challenge { CreateAuthChallenge(authKey, CurrentDefaultAuthSalt, CurrentDefaultAuthKey) },
			.salt { CurrentDefaultAuthSalt },
		};
		profile.state = UserProfile::State::Open;
		profile.version = CurrentAuthVersion;
		profile.recovery = {};

		return GetDatabase().UpdateProfile(profile) == DatabaseError::NoError;
	}

	bool UserManager::DeleteProfile(const fig::uuid& profileId)
	{
		if (auto profile = GetProfile(profileId))
		{
			// Sign out from current profile
			SignOut();

			if (GetDatabase().DeleteProfile(profileId) == DatabaseError::NoError)
			{
				remove(_profiles, profileId, [](auto&& p) { return p.id; });

				// Collect asset filenames
				std::unordered_set<fig::path> filenames;

				auto profilePath = (*profile).GetPath();

				{	// Scan index database
					IndexDatabase db(profilePath / std::format("{}.{}", Constants::Paths::AssetIndexFileName, Constants::Paths::AssetIndexFileExt));
					if (auto assets = db.FetchAssets())
					{
						filenames.insert_range(assets.value()
							| std::views::keys
							| std::views::transform([&profilePath](auto&& id) { 
								return profilePath / std::format("{}.{}", filename_from_uuid(id).u8string(), Constants::Paths::AssetFileExt);
							})
						);
					}
				}

				filenames.insert(profilePath / std::format("{}.{}", Constants::Paths::RecoveryFileName, Constants::Paths::RecoveryFileExt));
				filenames.insert(profilePath / std::format("{}.{}", Constants::Paths::ProfileImageFileName, Constants::Paths::ProfileImageFileExt));
				filenames.insert(profilePath / std::format("{}.{}", Constants::Paths::AssetIndexFileName, Constants::Paths::AssetIndexFileExt));
				filenames.insert(profilePath / Constants::Paths::UserSettings);

				// Delete content files
				size_t count = DeleteFiles(filenames);
				if (count > 0)
					LogLn(std::format("Deleted {} files", count));

				// Delete profile folder
				DeleteDirectory(profilePath);

				if (_profiles.empty())
					CreateDefaultProfile(); // Need at least one profile
				return true;
			}
		}
		return false;
	}

	fig::optional_cref<UserProfile> UserManager::GetActiveProfile() const noexcept
	{
		if (_pSignedInProfile != nullptr)
			return make_optional_cref(*_pSignedInProfile);
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
		if (_pSignedInProfile == nullptr or !_pContentManager)
			throw std::runtime_error("Not signed in");

		return static_cast<UserContentManager&>(*_pContentManager);
	}

	UserSettings& UserManager::GetSettings()
	{
		if (_pSignedInProfile == nullptr or !_pUserSettings)
			throw std::runtime_error("Not signed in");

		return static_cast<UserSettings&>(*_pUserSettings);
	}

	ProfileDatabase& UserManager::GetDatabase() noexcept
	{
		if (!_pProfileDB)
			_pProfileDB = std::make_unique<ProfileDatabase>(GetProfilesFilename(std::format("{}.{}", Constants::Paths::ProfilesFileName, Constants::Paths::ProfilesFileExt)));
		return *_pProfileDB.get();
	}

	std::optional<fig::string> UserManager::GenerateNewRecoveryKey()
	{
		if (not IsSignedIn())
			return std::nullopt;

		auto& profile = *_pSignedInProfile;

		AuthKey recoveryKey;
		AuthChallenge recoveryChallenge;
		if (CreateRecoveryFile(profile, GetActiveAuthKey(), recoveryChallenge, recoveryKey))
		{
			profile.recovery = UserAuth {
				.challenge = recoveryChallenge,
				.salt = profile.auth.salt,
			};

			if (_pProfileDB->UpdateProfile(profile) == DatabaseError::NoError)
			{
				auto code = RecoveryKeyToCode(recoveryKey);
				LogLn(std::format("Recovery code for user {}: {}", profile.id.to_str(), code));
				return code;
			}
		}
		return std::nullopt;
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

	bool UserManager::CreateRecoveryFile(const UserProfile& profile, const AuthKey& authKey, AuthChallenge& recoveryChallenge, AuthKey& recoveryKey)
	{
		auto& salt = profile.auth.salt;
		recoveryKey = RandomKey();
	
		AuthKey encKey = DeriveKeyFromBytes(recoveryKey, salt);
		recoveryChallenge = CreateAuthChallenge(authKey, salt, encKey);
		return AssetFileWriter::WriteRecoveryFile(profile, recoveryChallenge) == FileError::NoError;
	}

	bool UserManager::RecoverProfile(const fig::uuid& profileId, const fig::string& recoveryCode)
	{
		AuthKey recoveryKey;
		if (RecoveryCodeToKey(recoveryCode, recoveryKey))
			return RecoverProfile(profileId, recoveryKey);
		return false;
	}

	bool UserManager::RecoverProfile(const fig::uuid& profileId, const AuthKey& recoveryKey)
	{
		auto itProfile = std::find_if(_profiles.begin(), _profiles.end(), [&profileId](const UserProfile& profile) {
			return profile.id == profileId;
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
				.state {},
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

	bool UserManager::ChangeProfileName(const fig::uuid& profileId, const fig::string& newName)
	{
		if (empty_or_whitespace(newName))
			return false;

		if (auto profile = GetProfile(profileId))
		{
			profile->name = newName;
			if (_pProfileDB->UpdateProfile(*profile) == DatabaseError::NoError)
				return true;
		}

		return false;
	}

	FileError UserManager::ChangeProfilePicture(const UserProfile& profile, const fig::sdl::Surface& image)
	{
		auto filename = fig::path(std::format("{}.{}", Constants::Paths::ProfileImageFileName, Constants::Paths::ProfileImageFileExt));

		if (image.empty())
		{
			return DeleteFile(profile.GetPath() / filename);
		}

		if (auto profileImage = CreateProfileImage(image); not profileImage.empty() and profileImage->w > 0 and profileImage->h > 0)
		{
			auto pSurface = profileImage.get();

			fig::bytes data;
			if (SDL_LockSurface(pSurface))
			{
				size_t data_length = toUZ(pSurface->h * pSurface->pitch);

				// Copy pixel data
				data.resize(data_length);
				std::memcpy(data.data(), (fig::byte*)pSurface->pixels, data_length);
				SDL_UnlockSurface(pSurface);
			}
			else
			{
				return FileError::WriteError;
			}

			AssetFile image_file {
				.parent_id { profile.id },
				.type = make_asset_type(AssetType::Image, ImageAssetType::ProfileImage, DataFormat::ImageUncompressed),
				.data_length { data.size() },
				.data_encrypted { false },
				.data { std::move(data) },
			};

			auto now = fig::now();
			image_file.meta[MetaTag::CreatedAt] = now;
			image_file.meta[MetaTag::UpdatedAt] = now;
			image_file.meta[MetaTag::ImageWidth] = static_cast<uint16_t>(pSurface->w);
			image_file.meta[MetaTag::ImageHeight] = static_cast<uint16_t>(pSurface->h);
			image_file.meta[MetaTag::ImageFormat] = static_cast<uint8_t>(ImageFormat::RGB24);

			if (auto error = AssetFileWriter::WriteProfileFile(profile, filename, image_file); error != FileError::NoError)
				return error;
			return FileError::NoError;
		}
		else
		{
			return FileError::UnknownError;
		}
	}


	fig::sdl::Surface UserManager::GetProfileImage(const fig::user::UserProfile& profile)
	{
		if (auto result = AssetFileReader::ReadProfileFile(profile, std::format("{}.{}", Constants::Paths::ProfileImageFileName, Constants::Paths::ProfileImageFileExt)))
		{
			const auto& asset = result.value();
			auto width = asset.get_meta<uint16_t>(fig::io::MetaTag::ImageWidth).value_or(0);
			auto height = asset.get_meta<uint16_t>(fig::io::MetaTag::ImageHeight).value_or(0);
			auto format = static_cast<ImageFormat>(asset.get_meta<uint8_t>(fig::io::MetaTag::ImageFormat).value_or(0));
			if (auto image = CreateSurfaceFromBytes(width, height, format, asset.data))
				return image;
		}

		return {};
	}

}