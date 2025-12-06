#include "llm/ContextBlock.h"
#include "Constants.h"
#include <cassert>

llama_seq_id ContextBlock::get_any_sequence_id() const noexcept
{
	for (size_t i = 0; i < Constants::Context::AllSequenceIDs.size() && i < Constants::Context::MaxSequences; ++i)
	{
		if (sequenceId.IsSet(Constants::Context::AllSequenceIDs[i]))
			return static_cast<llama_seq_id>(i);
	}
	assert(0 && "Block has no sequence");
	return -1;
}

SequenceIndices ContextBlock::get_sequence_ids(int32_t n_seq_max) const noexcept
{
	SequenceIndices seqIds;
	seqIds.reserve(n_seq_max);

	for (size_t i = 0; i < Constants::Context::AllSequenceIDs.size() && i < n_seq_max; ++i)
	{
		if (sequenceId.IsSet(Constants::Context::AllSequenceIDs[i]))
			seqIds.push_back(toI(i));
	}
	return seqIds;
}
