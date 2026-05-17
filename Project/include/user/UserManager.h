#ifndef USER_MANAGER_H__
#define USER_MANAGER_H__
#pragma once

#include "Figment.h"
#include "user/UserProfile.h"
#include "io/AssetManager.h"
#include "io/ContentManager.h"
#include "user/UserSettings.h"
#include "user/ProfileDatabase.h"

namespace fig::user
{
	class UserManager
	{
	public:
		UserManager();
		virtual ~UserManager();

		bool LoadProfiles();
		std::optional<UserProfileCRef> GetProfile(const fig::uuid& id) const noexcept;
		std::optional<UserProfileRef> GetProfile(const fig::uuid& id) noexcept;
		const std::vector<UserProfile>& GetProfiles() const noexcept { return _profiles; };
		std::optional<UserProfileCRef> CreateDefaultProfile();
		std::optional<UserProfileCRef> CreateProfile(const fig::string& profileName, const fig::string& password);

		bool IsSignedIn() const noexcept { return _signedInProfile != nullptr; };
		bool SignIn(const fig::uuid& profileID, const fig::string& password);
		bool SignIn(const fig::string& profileName, const fig::string& password);
		bool SignInDefaultProfile();
		bool SignOut();
		
		const UserProfile& GetActiveProfile() const;
		fig::io::UserContentManager& GetContent();
		UserSettings& GetSettings();
		const fig::auth::AuthKey& GetActiveAuthKey() const noexcept { return _signedInAuthKey; };
		
		bool ChangePassword(const fig::uuid& profileID, const fig::string& oldPassword, const fig::string& newPassword);
		bool CreateRecoveryFile(const UserProfile& profile, const fig::string& password, fig::auth::AuthChallenge& recoveryChallenge, fig::auth::AuthKey& recoveryKey);
		bool RecoverProfile(const fig::uuid& profileID, const fig::string& recoveryCode);
		bool RecoverProfile(const fig::uuid& profileID, const fig::auth::AuthKey& recoveryKey);

	private:
		static bool Authenticate(const UserProfile& profile, const fig::string& password, fig::auth::AuthKey& outKey);
		static bool Authenticate(const fig::auth::AuthChallenge& challenge, const fig::auth::AuthSalt& salt, const fig::auth::AuthKey& key, fig::auth::AuthKey& outKey, fig::auth::AuthVersion version = fig::auth::CurrentAuthVersion);
		bool SignIn(UserProfile& profile, const fig::string& password);
		fig::io::ProfileDatabase& GetDatabase() noexcept;
		static fig::string RecoveryKeyToCode(const fig::auth::AuthKey& key, fig::auth::AuthVersion version = fig::auth::CurrentAuthVersion) noexcept;
		static bool RecoveryCodeToKey(const fig::string& code, fig::auth::AuthKey& outKey, fig::auth::AuthVersion version = fig::auth::CurrentAuthVersion) noexcept;

	private:
		std::unique_ptr<fig::io::ProfileDatabase> _pProfileDB;

		std::vector<UserProfile> _profiles {};
		UserProfile* _signedInProfile = nullptr;
		fig::auth::AuthKey _signedInAuthKey {};

		std::unique_ptr<fig::io::UserContentManager> _pContentManager;
		std::unique_ptr<UserSettings> _pUserSettings;
	};
}
#endif