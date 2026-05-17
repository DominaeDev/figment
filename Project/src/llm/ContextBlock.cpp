#include <pch.h>
#include "llm/ContextBlock.h"
#include <cassert>

using namespace fig::llm;

Sequence ContextBlock::get_any_sequence_id() const noexcept
{
	for (size_t i = 0; i < AllSequenceSlots.size() && i < Constants::DefaultModelSettings::MaxSequences; ++i)
	{
		if (sequenceSlots.IsSet(AllSequenceSlots[i]))
			return static_cast<llama_seq_id>(i);
	}
	assert(0 && "Block has no sequence");
	return -1;
}

Sequences ContextBlock::get_sequence_ids(int32_t n_seq_max) const noexcept
{
	Sequences seqIds;
	seqIds.reserve(n_seq_max);

	for (size_t i = 0; i < AllSequenceSlots.size() && i < n_seq_max; ++i)
	{
		if (sequenceSlots.IsSet(AllSequenceSlots[i]))
			seqIds.push_back(toI(i));
	}
	return seqIds;
}
