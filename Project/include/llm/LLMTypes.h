#pragma once

#include "Types.h"
#include <llama.h>

using ModelPtr = llama_model*;
using ContextPtr = llama_context*;
using VocabPtr = const llama_vocab*;
using SamplerPtr = llama_sampler*;
using Batch = llama_batch;
using Token = llama_token;
using LlamaSequence = llama_seq_id;
using SequenceIndices = std::vector<LlamaSequence>;

enum class Sequence : int32_t
{
	None	= 0,
	Bot1	= 1 << 0,
	Bot2	= 1 << 1,
	Bot3	= 1 << 2,
	Bot4	= 1 << 3,
	Shared	= Bot1 | Bot2 | Bot3 | Bot4,
	Default = Bot1,
};
using SequenceId = EnumFlags<Sequence>;

constexpr std::array<Sequence, 4> AllSequenceIDs {
	Sequence::Bot1,
	Sequence::Bot2,
	Sequence::Bot3,
	Sequence::Bot4,
};
