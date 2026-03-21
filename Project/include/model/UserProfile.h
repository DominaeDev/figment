#ifndef USER_PROFILE_H__
#define USER_PROFILE_H__
#pragma once

#include "Types.h"
#include "util/Security.h"
#include "Constants.h"
#include <array>

namespace fig::user
{
	struct UserProfile
	{
		unsigned short version { 0 };

		fig::uuid id { 0, 0 };
		fig::string name;
		fig::user::auth::UserAuth auth {};
		fig::user::auth::UserAuth recovery {};

		inline constexpr bool IsValid() const noexcept
		{
			return version == 0
				and not id.empty()
				and not name.empty();
		}

		fig::path GetPath() const noexcept
		{
			return fig::path(Constants::Paths::ProfilesFolder) / 
				fig::path(id.str()
					| std::ranges::views::filter([](char c) { return c != '-'; })
					| std::ranges::to<fig::string>());
		}
	};

	using UserProfileRef = std::reference_wrapper<UserProfile>;
	using UserProfileCRef = std::reference_wrapper<const UserProfile>;
}
#endif