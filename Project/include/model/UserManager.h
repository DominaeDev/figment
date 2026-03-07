#ifndef USER_MANAGER_H__
#define USER_MANAGER_H__
#pragma once

#include "Types.h"
#include "model/UserProfile.h"
#include "model/AssetManager.h"
#include "model/ContentDatabase.h"
#include "util/ProfileDatabase.h"

namespace fig::user
{
	class UserManager
	{
	public:
		UserManager();
		~UserManager() = default;

		bool LoadProfiles();

		std::optional<UserProfileCRef> CreateDefaultProfile();
		std::optional<UserProfileCRef> CreateProfile(const fig::string& profileName, const fig::string& password);

		bool IsSignedIn() const noexcept { return _signedInProfile != nullptr; };
		bool SignIn(const fig::uuid& profileID, const fig::string& password);
		bool SignIn(const fig::string& profileName, const fig::string& password);
		bool SignInDefaultProfile();
		bool SignOut();
		
		const UserProfile& GetActiveProfile() const;
		fig::io::AssetManager& GetProfileAssets();
		fig::io::ContentDatabase& GetContent();
		const fig::user::auth::AuthKey& GetActiveAuthKey() const noexcept { return _signedInAuthKey; };
		
		bool ChangePassword(const fig::uuid& profileID, const fig::string& oldPassword, const fig::string& newPassword);

		bool CreateRecoveryFile(const UserProfile& profile, const fig::string& password, fig::user::auth::AuthChallenge& recoveryChallenge, fig::user::auth::AuthKey& recoveryKey);
		
		bool RecoverProfile(const fig::uuid& profileID, const fig::string& recoveryCode);
		bool RecoverProfile(const fig::uuid& profileID, const fig::user::auth::AuthKey& recoveryKey);

	private:
		static bool Authenticate(const UserProfile& profile, const fig::string& password, fig::user::auth::AuthKey& outKey);
		static bool Authenticate(const fig::user::auth::AuthChallenge& challenge, const fig::user::auth::AuthSalt& salt, const fig::user::auth::AuthKey& key, fig::user::auth::AuthKey& outKey);
		bool SignIn(UserProfile& profile, const fig::string& password);
		fig::io::ProfileDatabase& GetDatabase() noexcept;
		static fig::string RecoveryKeyToCode(const fig::user::auth::AuthKey& key) noexcept;
		static bool RecoveryCodeToKey(const fig::string& code, fig::user::auth::AuthKey& outKey) noexcept;

	private:
		std::unique_ptr<fig::io::ProfileDatabase> _pProfileDB;

		std::vector<UserProfile> _profiles {};
		UserProfile* _signedInProfile = nullptr;
		fig::user::auth::AuthKey _signedInAuthKey {};

		std::unique_ptr<fig::io::AssetManager> _pAssetMngr;
		std::unique_ptr<fig::io::ContentDatabase> _pContentDatabase;
	};
}
#endif