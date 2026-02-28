#pragma once

#include <stdint.h>

enum class DatabaseError : uint32_t
{
	NoError = 0,
	NotConnected,
	ZeroChanges,
	FailedContraint,
	SQLError,
};
