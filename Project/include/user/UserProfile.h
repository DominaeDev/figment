#pragma once

#include "Figment.h"
#include "user/Security.h"

namespace fig::user
{
	struct UserProfile
	{
		fig::auth::AuthVersion version { uint16_t(-1) };

		fig::uuid id {};
		fig::string name;
		fig::auth::UserAuth auth {};
		fig::auth::UserAuth recovery {};

		enum class State : uint8_t
		{
			Open		= 0x00,
			Locked		= 0x01,
			Recovered	= 0x02,
		};

		State state {};

		inline constexpr bool IsValid() const noexcept
		{
			return fig::auth::IsValidAuthVersion(version)
				and not id.empty()
				and not name.empty();
		}

		fig::path GetPath() const;
	};
}
