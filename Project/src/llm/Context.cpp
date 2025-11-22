#include "llm/Context.h"
#include "llm/LLMUtility.h"
#include "util/Common.h"
#include "Constants.h"
#include <cassert>
#include <algorithm>
#include <format>
#include <cassert>

static void copy_batch_tokens(llama_batch& batch, int32_t from, int32_t to)
{
	batch.token[to] = batch.token[from];
	batch.n_seq_id[to] = batch.n_seq_id[from];
	for (int32_t itSeq = 0; itSeq < Constants::Context::MaxSequences; ++itSeq)
		batch.seq_id[to][itSeq] = batch.seq_id[from][itSeq];
	batch.logits[to] = batch.logits[from];
}

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

SequenceIndices ContextState::get_all_sequences() const noexcept
{
	SequenceIndices result;
	result.reserve(num_sequences);
	for (int32_t i = 0; i < num_sequences; ++i)
		result.push_back(i);
	return result;
}

bool ContextState::ReserveTokens(int32_t ctx_reserve, bool bForce)
{
	int n_ctx_used = llama_kv_self_used_cells(pCtx);
	int32_t ctx_size = llama_n_ctx(pCtx);
	if (n_ctx_used + ctx_reserve >= ctx_size || bForce)
	{
		DebugPrintLn(">> REALLOCATING CONTEXT");
		int32_t shift = sequence.AllocateKVCache(ctx_reserve);
		DebugPrintLn(std::format(">> Allocated {} tokens.", std::abs(shift), ctx_size - sequence.cursor_pos));
		assert(ctx_size - sequence.cursor_pos >= Constants::Context::MaxResponseLength);
		return true;
	}
	return false;
}

void ContextSequence::RefreshBlockPositions()
{
	int32_t offset = 0;
	for (auto& block : blocks)
	{
		if (offset >= chat_begin_pos)
			block.offset = offset;
		if (CheckEnumFlag(block.flags, ContextBlockFlag::Static))
			offset = std::max(offset, block.offset + block.length());
		else
			offset += block.length();
	}
}

int32_t ContextSequence::DecodeUncached(int32_t cursor_pos)
{
	for (auto& block : blocks)
	{
		if (CheckEnumFlag(block.flags, ContextBlockFlag::Cached))
			continue;

		std::vector<llama_seq_id> seqs = block.get_sequence_ids();
		llama_batch batch_view = llm_util::create_batch(block.tokens, seqs, block.offset);
		if (!llama_decode(pCtx, batch_view))
		{
			block.flags = block.flags | ContextBlockFlag::Cached;
			cursor_pos = std::max(cursor_pos, block.offset + block.length());
			llama_batch_free(batch_view);
		}
		else
		{
			llama_batch_free(batch_view);
			return -1; // Error
		}
	}
	return cursor_pos;
}

bool ContextSequence::RebuildKVCache()
{
#if FALSE // Clear everything
	llama_kv_self_clear(pCtx);
	int r = llama_decode(pCtx, batch);
#else // Clear non-static only
	int32_t blocks_pos = chat_begin_pos;

	for (size_t i = 0; i < Constants::Context::AllSequenceIDs.size(); ++i)
		llama_kv_self_seq_rm(pCtx, (int32_t)i, blocks_pos, -1);

	auto batch_view = llm_util::create_batch_view(batch, blocks_pos, batch.n_tokens - blocks_pos);
	int r = llama_decode(pCtx, batch_view);
#endif

	cursor_pos = batch.n_tokens;
	return r == 0;
}

int32_t ContextSequence::RemoveBlocksAndShift(int32_t idx_from, int32_t idx_to) // [from, to]
{
	// Remove
	int32_t shift_amount = 0;
	for (auto i = idx_from; i <= idx_to; ++i)
		shift_amount += blocks[i].length();
	if (shift_amount == 0)
		return 0;

#if _DEBUG
	int32_t n_used = llama_kv_self_used_cells(pCtx);
#endif
	int32_t pos_remove_begin = blocks[idx_from].offset;
	int32_t pos_remove_end = pos_remove_begin + shift_amount;

	// Remove and shift
	for (int32_t i = idx_to; i >= idx_from; --i)
	{
		auto& block = blocks[i];
		assert(block.sequenceId != SequenceId::None);
		assert(block.offset >= pos_remove_begin && block.offset + block.length() <= pos_remove_end);

		if (!llama_kv_self_seq_rm(pCtx, -1, block.offset, block.offset + block.length()))
			return 0;

		blocks.erase(blocks.begin() + i);
	}

	// Shift up block-wise because of sequences
	for (size_t i = idx_from; i < blocks.size(); ++i)
	{
		int32_t seq_id = blocks[i].get_sequence_ids()[0]; // Any of the block's sequence ids will work
		llama_kv_self_seq_add(pCtx, 0, blocks[i].offset, blocks[i].offset + blocks[i].length(), -shift_amount); // seq_id (ugh)
		blocks[i].offset -= shift_amount;
	}

	llama_kv_self_defrag(pCtx);
	llama_kv_self_update(pCtx);

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
		for (int32_t itSeq = 0; itSeq < Constants::Context::MaxSequences; ++itSeq)
			batch.seq_id[pos_remove_begin + i][itSeq] = batch.seq_id[pos_remove_end + i][itSeq];
		batch.logits[pos_remove_begin + i] = false;
	}
	batch.n_tokens -= shift_amount;

	return (int32_t)-shift_amount;
}

int32_t ContextSequence::EraseTokens(int32_t from, int32_t length)
{
	if (from < 0 || from >= Constants::Context::Size || length <= 0  || from + length > Constants::Context::Size)
		return 0;

	llama_kv_self_seq_rm(pCtx, -1, from, from + length);

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
	llama_kv_self_seq_rm(pCtx, -1, begin, end);
	llama_kv_self_seq_add(pCtx, 0, end, -1, -n_removed); //! @seq_id
	llama_kv_self_update(pCtx);
	assert(false && "This doesn't handle sequences yet.");

	// Update batch
	int32_t n_batch = batch.n_tokens;
	for (int32_t i = 0; i < n_removed; ++i)
	{
		batch.token[begin + i] = batch.token[end + i];
		batch.pos[begin + i] = begin + i;
		batch.n_seq_id[begin + i] = batch.n_seq_id[end + i];
		for (int32_t itSeq = 0; itSeq < Constants::Context::MaxSequences; ++itSeq)
			batch.seq_id[begin + i][itSeq] = batch.seq_id[end + i][itSeq];
		batch.logits[begin + i] = false;
	}
	batch.n_tokens -= n_removed;
	return n_removed;
}

int32_t ContextSequence::BatchAllocate(int32_t pos, int32_t length)
{
	// Remove
	llama_kv_self_seq_add(pCtx, 0, pos, -1, length); //! @seq_id
	llama_kv_self_update(pCtx);
	assert(false && "This doesn't handle sequences yet.");

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
		for (int32_t itSeq = 0; itSeq < Constants::Context::MaxSequences; ++itSeq)
			batch.seq_id[idx][itSeq] = batch.seq_id[idx - length][itSeq];
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
		for (int32_t itSeq = 0; itSeq < Constants::Context::MaxSequences; ++itSeq)
			batch.seq_id[idx][itSeq] = -1;
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
			for (int32_t itSeq = 0; itSeq < Constants::Context::MaxSequences; ++itSeq)
				batch.seq_id[idx][itSeq] = batch.seq_id[src_pos + i][itSeq];
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
			for (int32_t itSeq = 0; itSeq < Constants::Context::MaxSequences; ++itSeq)
				batch.seq_id[idx][itSeq] = batch.seq_id[src_pos + len - i - 1][itSeq];
			batch.logits[idx] = batch.logits[src_pos + len - i - 1];
		}		
	}

	// Apply down-shifts
	llama_kv_self_seq_add(pCtx, 0, pos, -1, offset); //! @seq_id
	assert(false && "This doesn't handle sequences yet.");

	return offset;
}

int32_t ContextSequence::BatchWrite(std::span<llama_token> tokens, SequenceId seq_id, int32_t pos)
{
	// Add to context batch
	auto seq_ids = llm_util::get_sequence_indices(seq_id);
	int32_t n_seq = toI(seq_ids.size());
	int32_t n_tokens = toI(tokens.size());
	for (int32_t i = 0; i < n_tokens; ++i)
	{
		int idx = pos + i;
		batch.token[idx] = tokens[i];
		batch.pos[idx] = idx;
		batch.n_seq_id[idx] = n_seq;
		for (size_t itSeq = 0; itSeq < seq_ids.size(); ++itSeq)
			batch.seq_id[idx][itSeq] = seq_ids[itSeq];
		batch.logits[idx] = false;
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

void ContextSequence::BatchSetSequences(int32_t from, int32_t length, SequenceId seq_id)
{
	auto seqIds = llm_util::get_sequence_indices(seq_id);
	int32_t n_seq = toI(seqIds.size());
	int32_t to = from + length;
	for (int32_t pos = from; pos < to; ++pos)
	{
		batch.n_seq_id[pos] = n_seq;
		for (size_t i = 0; i < seqIds.size() && i < Constants::Context::MaxSequences; ++i)
			batch.seq_id[pos][i] = seqIds[i];
	}
}

int32_t ContextSequence::AllocateKVCache(int32_t min_reserve)
{
	// Allocate and shift context window
	int n_ctx_used = llama_kv_self_used_cells(pCtx);
	int32_t ctx_size = llama_n_ctx(pCtx);

	int32_t ctx_chat_max = ctx_size - chat_begin_pos; // Exclude system prompt
	int32_t free_tokens = std::max(min_reserve, toI(ctx_chat_max * (1.0f - Constants::Context::WindowKeepRatio)));
		
	auto itFirst = std::find_if(blocks.begin(), blocks.end(), [](auto& block) { return !block.is_static(); });
	assert(itFirst != blocks.end());

	int32_t total = 0;
	auto itLast = itFirst;
	while (itLast != blocks.end() && total < free_tokens && itLast->is_cached())
	{
		total += itLast->length();
		itLast++;
	}

	assert(itFirst != itLast);

	if (itFirst != itLast)
	{
		int32_t shift = RemoveBlocksAndShift(toI(std::distance(blocks.begin(), itFirst)), toI(std::distance(blocks.begin(), itLast)) - 1);

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
			int32_t shift = RemoveBlocksAndShift(i, i);
			cursor_pos += shift;
			response_pos += shift;
			offset += shift;
		}
	}
	return offset;
}

int32_t ContextSequence::GetBlockAppendOffset() const
{
	int32_t offset = 0;
	for (auto& block : blocks)
		offset = std::max(offset, block.offset + block.length());
	return offset;
}