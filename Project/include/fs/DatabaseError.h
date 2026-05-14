#pragma once

#include <stdint.h>

namespace fig::io
{
	enum class DatabaseError : uint32_t
	{
		NoError = 0,
		NotConnected,
		ZeroChanges,
		FailedContraint,
		SQLError,
	};

	inline constexpr bool Success(DatabaseError error) { return error == DatabaseError::NoError; };
}