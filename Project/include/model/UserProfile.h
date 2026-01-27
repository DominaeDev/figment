#ifndef USER_PROFILE_H__
#define USER_PROFILE_H__
#pragma once

#include "Types.h"
#include "util/Security.h"
#include <array>

namespace fig::fs
{
	struct UserProfile
	{
		fig::uuid id { 0, 0 };
		fig::string name;
		fig::bytes authChallenge {};
		fig::security::AuthSalt authSalt {};
		unsigned short version { 0 };

		inline constexpr bool IsValid() const noexcept
		{
			return version == 0
				and not id.empty()
				and not name.empty()
				and authChallenge.size() == 32;
		}

		fig::path GetPath() const noexcept
		{
			return fig::path(Constants::Paths::ProfilesFolder) / 
				fig::path(id.str()
					| std::ranges::views::filter([](char c) { return c != '-'; })
					| std::ranges::to<fig::string>());
		}
	};
}
#endif