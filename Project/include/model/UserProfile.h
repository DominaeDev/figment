#ifndef USER_PROFILE_H__
#define USER_PROFILE_H__
#pragma once

#include <array>
#include "Types.h"
#include "util/Encrypt.h"

namespace fig::fs
{
	struct UserProfile
	{
		fig::uuid id { 0 };
		fig::string name;
		fig::bytes authChallenge {};
		fig::security::AuthSalt authSalt {};
		unsigned short version { 0 };
	};
}
#endif