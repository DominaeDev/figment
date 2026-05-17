#ifndef USER_PROFILE_H__
#define USER_PROFILE_H__
#pragma once

#include "Figment.h"
#include "user/Security.h"
#include <array>

namespace fig::user
{
	struct UserProfile
	{
		fig::auth::AuthVersion version { uint16_t(-1) };

		fig::uuid id { 0, 0 };
		fig::string name;
		fig::auth::UserAuth auth {};
		fig::auth::UserAuth recovery {};
		bool has_password {};

		inline constexpr bool IsValid() const noexcept
		{
			return fig::auth::IsValidAuthVersion(version)
				and not id.empty()
				and not name.empty();
		}

		fig::path GetPath() const noexcept
		{
			return fig::path(Constants::Paths::ProfilesFolder) / 
				fig::path(id.to_str()
					| std::ranges::views::filter([](char c) { return c != '-'; })
					| std::ranges::to<fig::string>());
		}
	};

	using UserProfileRef = std::reference_wrapper<UserProfile>;
	using UserProfileCRef = std::reference_wrapper<const UserProfile>;
}
#endif