#ifndef USER_PROFILE_H__
#define USER_PROFILE_H__
#pragma once

#include <array>
#include "Types.h"
#include "util/Security.h"

namespace fig::fs
{
	struct UserProfile
	{
		fig::uuid id { 0, 0 };
		fig::string name;
		fig::bytes authChallenge {};
		fig::security::AuthSalt authSalt {};
		unsigned short version { 0 };

		constexpr bool is_valid() const
		{
			return version == 0
				and not id.empty()
				and not name.empty()
				and authChallenge.size() == 48;
		}
	};
}
#endif