#include "llm/Context.h"
#include "llm/LLMUtility.h"
#include "Constants.h"
#include <cassert>
#include <algorithm>
#include <format>

int32_t ContextSequence::default_seq_id = 0;

SequenceIndices ContextBlock::get_sequence_ids() const noexcept
{
	SequenceIndices seqIds;
	seqIds.reserve(Constants::Context::MaxSequences);

	for (size_t i = 0; i < Constants::Context::AllSequenceIDs.size() && i < Constants::Context::MaxSequences; ++i)
	{
		if ((bool)(this->sequenceId & Constants::Context::AllSequenceIDs[i]))
			seqIds.push_back(toI(i));
	}
	return seqIds;
}

int32_t ContextState::get_max_position() const
{
	int32_t max = 0;
	for (auto& block : sequence.blocks)
	{
		if (block.is_static() || block.is_cached())
		{
			max = std::max(max, block.offset + toI(block.length()));
			continue;
		}
		break;
	}
	return max;
}

SequenceIndices ContextState::get_active_sequence() const noexcept
{
	return llm_util::get_sequence_indices(llm_util::sequence_from_index(active_sequence));
}

SequenceIndices ContextState::get_all_sequences() const noexcept
{
	SequenceIndices result;
	result.reserve(num_sequences);
	for (int32_t i = 0; i < num_sequences; ++i)
		result.push_back(i);
	return result;
}

void ContextSequence::AssignBlockPositions()
{
	int32_t block_offset = 0;

	for (auto& block : blocks)
	{
		block.offset = block_offset;
		block_offset += block.length();
	}
}

bool ContextSequence::RebuildKVCache()
{
#if FALSE // Clear everything
	llama_kv_self_clear(pCtx);
	int r = llama_decode(pCtx, batch);
#else // Clear non-static only
	int32_t blocks_pos = GetFirstNonStaticOffset();

	for (size_t i = 0; i < Constants::Context::AllSequenceIDs.size(); ++i)
		llama_kv_self_seq_rm(pCtx, (int32_t)i, blocks_pos, -1);

	auto batch_view = llm_util::create_batch_view(batch, blocks_pos, batch.n_tokens - blocks_pos);
	int r = llama_decode(pCtx, batch_view);
#endif

	cursor_pos = batch.n_tokens;
	return r == 0;
}

int32_t ContextSequence::GetFirstNonStaticOffset() const
{
	int32_t offset = 0;

	for (auto& block : blocks)
	{
		if (!block.is_static())
			break;
		offset += block.length();
	}
	return offset;
}

int32_t ContextSequence::RemoveAndShift(std::vector<ContextBlock>::iterator itBegin, std::vector<ContextBlock>::iterator itEnd)
{
	// Remove
	int32_t shift_amount = 0;
	for (auto it = itBegin; it != itEnd; ++it)
		shift_amount += (*it).length();
	if (shift_amount == 0)
		return 0;

#if _DEBUG
	int32_t n_used = llama_kv_self_used_cells(pCtx);
#endif
	int32_t pos_remove_begin = (*itBegin).offset;
	int32_t pos_remove_end = pos_remove_begin + shift_amount;

	// Remove and shift
	for (auto itBlock = itBegin; itBlock != itEnd; ++itBlock)
	{
		assert((*itBlock).sequenceId != SequenceId::None);
		for (auto seq_id : llm_util::get_sequence_indices((*itBlock).sequenceId))
		{
			if (!llama_kv_self_seq_rm(pCtx, seq_id, pos_remove_begin, pos_remove_end))
				return 0;
			llama_kv_self_seq_add(pCtx, seq_id, pos_remove_end, cursor_pos, -shift_amount);
		}
	}
	llama_kv_self_update(pCtx);

	blocks.erase(itBegin, itEnd);
	AssignBlockPositions();

#if _DEBUG
	int32_t n_used_after = llama_kv_self_used_cells(pCtx);
	assert(n_used_after < n_used);
#endif

	// Update batch
	for (int32_t i = 0; i < pos_remove_end - pos_remove_begin; ++i)
	{
		batch.pos[pos_remove_begin + i] = pos_remove_begin + i;
		batch.token[pos_remove_begin + i] = batch.token[pos_remove_end + i];
		batch.n_seq_id[pos_remove_begin + i] = batch.n_seq_id[pos_remove_end + i];
		batch.seq_id[pos_remove_begin + i][0] = batch.seq_id[pos_remove_end + i][0];
		batch.logits[i] = false;
	}
	batch.n_tokens -= shift_amount;

	return (int32_t)-shift_amount;
}

int32_t ContextSequence::EraseTokens(int32_t from, int32_t length)
{
	if (from < 0 || from >= Constants::Context::Size || length <= 0  || from + length > Constants::Context::Size)
		return 0;

	llama_kv_self_seq_rm(pCtx, default_seq_id, from, from + length);

	// Update batch
	for (int32_t i = 0; i < length; ++i)
	{
		int32_t idx = from + i;
		batch.pos[idx] = 0;
		batch.token[idx] = 0;
		batch.n_seq_id[idx] = 0;
		for (int32_t itSeq = 0; itSeq != Constants::Context::MaxSequences; ++itSeq)
			batch.seq_id[idx][itSeq] = -1;
		batch.logits[idx] = false;
	}
	batch.n_tokens -= length;
	return length;
}

int32_t ContextSequence::BatchRemove(int32_t begin, int32_t end)
{
	int32_t n_removed = end - begin;

	// Remove
	llama_kv_self_seq_rm(pCtx, default_seq_id, begin, end);
	llama_kv_self_seq_add(pCtx, default_seq_id, end, -1, -n_removed);
	llama_kv_self_update(pCtx);

	// Update batch
	int32_t n_batch = batch.n_tokens;
	for (int32_t i = 0; i < n_removed; ++i)
	{
		batch.token[begin + i] = batch.token[end + i];
		batch.n_seq_id[begin + i] = batch.n_seq_id[end + i];
		batch.pos[begin + i] = begin + i;
		batch.seq_id[begin + i][0] = batch.seq_id[end + i][0];
		batch.logits[i] = false;
	}
	batch.n_tokens -= n_removed;
	return n_removed;
}

int32_t ContextSequence::BatchAllocate(int32_t pos, int32_t length)
{
	// Remove
	llama_kv_self_seq_add(pCtx, default_seq_id, pos, -1, length);
	llama_kv_self_update(pCtx);

	int32_t ctx_size = llama_n_ctx(pCtx);

	// Update batch
	int32_t n_batch = batch.n_tokens;
	for (int32_t i = 0; i < n_batch - pos; ++i)
	{
		if (i >= ctx_size)
			continue;

		int32_t idx = n_batch + length - i - 1;
		batch.pos[idx] = idx;
		batch.token[idx] = batch.token[idx - length];
		batch.n_seq_id[idx] = batch.n_seq_id[idx - length];
		batch.seq_id[idx][0] = batch.seq_id[idx - length][0];
		batch.logits[idx] = false;
	}
	batch.n_tokens += length;

	// Clear allocated tokens
	for (int32_t i = 0; i < length; ++i)
	{
		int32_t idx = pos + i;
		batch.pos[idx] = -1;
		batch.token[idx] = 0;
		batch.n_seq_id[idx] = 0;
		batch.logits[idx] = false;
	}

	return length;
}

int32_t ContextSequence::ShiftTokens(int32_t pos, int32_t len, int32_t offset)
{
	// Shift down
	int32_t ctx_size = llama_n_ctx(pCtx);
	if (len < 0)
		len = ctx_size - pos;

	int32_t src_pos = pos;
	int32_t dest_pos = pos + offset;
	if (src_pos > dest_pos) // Shifting up, write top down
	{
		for (int32_t i = 0; i < offset; ++i)
		{
			int idx = dest_pos + i;
			if (idx < 0 || idx >= ctx_size)
				continue;

			batch.pos[idx] = idx;
			batch.token[idx] = batch.token[src_pos + i];
			batch.n_seq_id[idx] = batch.n_seq_id[src_pos + i];
			batch.seq_id[idx][0] = batch.seq_id[src_pos + i][0];
			batch.logits[idx] = batch.logits[src_pos + i];
		}
	}
	else if (src_pos < dest_pos) // Shifting down, write bottom up
	{
		for (int32_t i = 0; i < offset; ++i)
		{
			int idx = dest_pos + len - i - 1;
			if (idx < 0 || idx >= ctx_size)
				continue;

			batch.pos[idx] = idx;
			batch.token[idx] = batch.token[src_pos + len - i - 1];
			batch.n_seq_id[idx] = batch.n_seq_id[src_pos + len - i - 1];
			batch.seq_id[idx][0] = batch.seq_id[src_pos + len - i - 1][0];
			batch.logits[idx] = batch.logits[src_pos + len - i - 1];
		}		
	}

	// Apply down-shifts
	llama_kv_self_seq_add(pCtx, default_seq_id, pos, -1, offset);
	return offset;
}

int32_t ContextSequence::BatchWrite(std::span<llama_token> tokens, SequenceId seq_id, int32_t pos)
{
	// Add to context batch
	int32_t n_tokens = (int32_t)tokens.size();
	auto seq_ids = llm_util::get_sequence_indices(seq_id);
	for (int32_t i = 0; i < n_tokens; ++i)
	{
		int idx = pos + i;
		batch.token[idx] = tokens[i];
		batch.pos[idx] = idx;
		batch.n_seq_id[idx] = toI(seq_ids.size());
		for (size_t i = 0; i < seq_ids.size(); ++i)
			batch.seq_id[idx][i] = seq_ids[i];
		batch.logits[i] = false;
	}
	batch.n_tokens = pos + n_tokens;
	return n_tokens;
}

void ContextSequence::BatchSetSequences(int32_t pos, const std::vector<int32_t>& seqIds)
{
	batch.n_seq_id[pos] = toI(seqIds.size());
	for (size_t i = 0; i < seqIds.size() && i < Constants::Context::MaxSequences; ++i)
		batch.seq_id[pos][i] = seqIds[i];
}

int32_t ContextSequence::AllocateKVCache(int32_t alloc_size)
{
	// Allocate and shift context window
	int n_ctx_used = llama_kv_self_used_cells(pCtx);
	size_t ctx_reserve = std::max(alloc_size + Constants::Context::MaxResponseLength, Constants::Context::MicroBatchSize);
	size_t ctx_size = llama_n_ctx(pCtx);

	int32_t blocks_pos = GetFirstNonStaticOffset();
	size_t ctx_chat_max = ctx_size - blocks_pos; // Exclude system prompt
	size_t free_tokens = std::max(static_cast<int32_t>(ctx_reserve), static_cast<int32_t>(ctx_chat_max * (1.0f - Constants::Context::WindowKeepRatio)));
		
	auto itFirst = std::find_if(blocks.begin(), blocks.end(), [](auto& block) { return !block.is_static(); });
	assert(itFirst != blocks.end());

	size_t total = 0;
	auto itLast = itFirst;
	while (itLast != blocks.end() && total < free_tokens && itLast->is_cached())
	{
		total += itLast->length();
		itLast++;
	}

	assert(itFirst != itLast);

	if (itFirst != itLast)
	{
		int32_t shift = RemoveAndShift(itFirst, itLast);

		cursor_pos += shift;
		response_pos += shift;

		assert(ctx_size - cursor_pos >= Constants::Context::MaxResponseLength);
		return shift;
	}
	return 0;
}

int32_t ContextSequence::DecrementTTL(int32_t time)
{
	if (time <= 0)
		return 0;

	int32_t offset = 0;
	for (int32_t i = (int32_t)blocks.size() - 1; i >= 0; --i)
	{
		auto& block = blocks[i];
		if (!block.is_temporary())
			continue;

		block.ttl -= time;
		if (block.ttl > 0)
			continue;
		
		if (block.is_cached())
		{
			// Remove from context
			int32_t shift = RemoveAndShift(
				blocks.begin() + (ptrdiff_t)i,
				blocks.begin() + (ptrdiff_t)(i + 1));
			cursor_pos += shift;
			response_pos += shift;
			offset += shift;
		}
	}
	return offset;
}
