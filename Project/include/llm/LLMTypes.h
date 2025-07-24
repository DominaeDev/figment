#pragma once

#include "Types.h"

enum class ContextSequenceId : uint8_t
{
	None	= 0,
	Shared	= 1 << 0,
	Bot1	= 1 << 1,
	Bot2	= 1 << 2,
	Bot3	= 1 << 3,
	Bot4	= 1 << 4,
};
DEFINE_ENUM_FLAGS(ContextSequenceId, uint8_t);

constexpr std::array<ContextSequenceId, 5> AllContextSequenceIds {
	ContextSequenceId::Shared,
	ContextSequenceId::Bot1,
	ContextSequenceId::Bot2,
	ContextSequenceId::Bot3,
	ContextSequenceId::Bot4,
};

using ContextSequenceList = std::vector<int32_t>;