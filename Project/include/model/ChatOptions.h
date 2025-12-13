#ifndef CHAT_OPTIONS_H__
#define CHAT_OPTIONS_H__

#pragma once

#include "Types.h"

enum class LLMOption : uint32_t
{
	UseCharacterIds = 1 << 0,
	AllowUserResponse = 1 << 1,
	LimitMessages = 1 << 2,
	RandomizeMessageCount = 1 << 3,
	GreetUser = 1 << 4,
	Embeddings = 1 << 5,
	Uncensored = 1 << 6,

	StateVariables = 1 << 7,
	ReportStateChanges = 1 << 8,

	UseMultipleSequences = 1 << 9,
};
using LLMOptions = EnumFlags<LLMOption>;

#endif