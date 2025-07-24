#pragma once

#include "Types.h"

enum class ContextSequenceId : uint8_t
{
	None	= 0,
	Bot1	= 1 << 0,
	Bot2	= 1 << 1,
	Bot3	= 1 << 2,
	Bot4	= 1 << 3,
	Shared = Bot1 | Bot2 | Bot3 | Bot4,
};
DEFINE_ENUM_FLAGS(ContextSequenceId, uint8_t);


using ContextSequenceList = std::vector<int32_t>;