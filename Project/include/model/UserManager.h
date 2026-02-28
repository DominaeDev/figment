#ifndef USER_MANAGER_H__
#define USER_MANAGER_H__
#pragma once

#include "Types.h"
#include "model/UserProfile.h"
#include "model/AssetManager.h"
#include "model/ContentDatabase.h"
#include "util/ProfileDatabase.h"

namespace fig::fs
{
	class UserManager
	{
	public:
		UserManager() = default;
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
		AssetManager& GetProfileAssets();
		ContentDatabase& GetContent();
		const fig::security::AuthKey& GetActiveAuthKey() const noexcept { return _signedInAuthKey; };
		
		bool ChangePassword(const fig::uuid& profileID, const fig::string& oldPassword, const fig::string& newPassword);

	private:
		static bool Authenticate(const UserProfile& profile, const fig::string& password, fig::security::AuthKey& outKey);
		bool SignIn(UserProfile& profile, const fig::string& password);
		fig::user::ProfileDatabase& GetDatabase() noexcept;


	private:
		std::unique_ptr<fig::user::ProfileDatabase> _pProfileDB;

		std::vector<UserProfile> _profiles {};
		UserProfile* _signedInProfile = nullptr;
		fig::security::AuthKey _signedInAuthKey {};

		std::unique_ptr<AssetManager> _pAssetMngr;
		std::unique_ptr<ContentDatabase> _pContentDatabase;
	};
}
#endif