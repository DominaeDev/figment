#ifndef USER_MANAGER_H__
#define USER_MANAGER_H__
#pragma once

#include "Types.h"
#include "model/UserProfile.h"
#include "model/AssetManager.h"

namespace fig::fs
{
	class UserManager
	{
	public:
		UserManager() = default;
		~UserManager() = default;

		bool LoadProfiles();
		bool SaveProfiles() const;

		UserProfile& CreateDefaultProfile();
		UserProfile& CreateProfile(const fig::string& profileName, const fig::string& password);

		bool IsSignedIn() const noexcept { return _signedInProfile != nullptr; };
		bool SignIn(const fig::uuid& profileID, const fig::string& password);
		bool SignIn(const fig::string& profileName, const fig::string& password);
		bool SignInDefaultProfile();
		bool SignOut();
		
		const UserProfile& GetActiveProfile() const;
		AssetManager& GetProfileAssets();
		const fig::security::AuthKey& GetActiveAuthKey() const noexcept { return _signedInAuthKey; };
		
		bool ChangePassword(const fig::uuid& profileID, const fig::string& oldPassword, const fig::string& newPassword);

	private:
		static bool Authenticate(const UserProfile& profile, const fig::string& password, fig::security::AuthKey& outKey);
		bool SignIn(UserProfile& profile, const fig::string& password);

	private:
		std::vector<UserProfile> _profiles {};
		UserProfile* _signedInProfile = nullptr;
		fig::security::AuthKey _signedInAuthKey {};

		std::unique_ptr<AssetManager> _pAssetMngr;
	};
}
#endif