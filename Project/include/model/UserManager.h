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

		UserProfile& CreateProfile(fig::string name, fig::string password = "");

	private:
		std::vector<UserProfile> _profiles {};
	};
}
#endif