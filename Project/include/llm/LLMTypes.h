#pragma once

#include "Types.h"
#include "ContextCursor.h"
#include <llama.h>

namespace fig::llm
{
	// Aliases
	using ModelPtr = llama_model*;
	using ContextPtr = llama_context*;
	using VocabPtr = const llama_vocab*;
	using SamplerPtr = llama_sampler*;
	using Batch = llama_batch;
	using Token = llama_token;
	using Sequence = llama_seq_id;
	using Sequences = std::vector<Sequence>;

	enum class SequenceSlot : int32_t
	{
		None = 0,
		Bot1 = 1 << 0,
		Bot2 = 1 << 1,
		Bot3 = 1 << 2,
		Bot4 = 1 << 3,
		Bot5 = 1 << 4,
		Bot6 = 1 << 5,
		Bot7 = 1 << 6,
		Bot8 = 1 << 7,
		Shared = Bot1 | Bot2 | Bot3 | Bot4 | Bot5 | Bot6 | Bot7 | Bot8,
		Default = Bot1,
	};
	using SequenceSlots = EnumFlags<SequenceSlot>;
	constexpr Sequence InvalidSequence = -1;

	constexpr std::array<SequenceSlot, 4> AllSequenceSlots {
		SequenceSlot::Bot1,
		SequenceSlot::Bot2,
		SequenceSlot::Bot3,
		SequenceSlot::Bot4,
	};

	class LLMEngine;
	using LLMEnginePtr = std::shared_ptr<LLMEngine>;
	class LLMInstance;
	using LLMInstancePtr = std::shared_ptr<LLMInstance>;

	enum class LLMCursor : uint32_t;
}