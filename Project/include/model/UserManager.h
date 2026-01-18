#ifndef USER_MANAGER_H__
#define USER_MANAGER_H__
#pragma once

#include "Types.h"
#include "model/UserProfile.h"

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

		bool IsSignedIn() const noexcept { return not _signedInProfileId.empty(); };
		bool SignIn(const fig::uuid& profileID, const fig::string& password);
		bool SignIn(const fig::string& profileName, const fig::string& password);
		bool SignInDefaultProfile();
		bool SignOut();

		const fig::security::AuthKey& GetAuthKey() const noexcept { return _signedInAuthKey; };

	private:
		bool SignIn(const UserProfile& profile, const fig::string& password);

	private:
		std::vector<UserProfile> _profiles {};
		fig::uuid _signedInProfileId {};
		fig::security::AuthKey _signedInAuthKey {};
	};
}
#endif