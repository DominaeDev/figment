#include "llm/Context.h"
#include "llm/LLMUtility.h"
#include "Constants.h"
#include <cassert>

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

	llama_kv_self_seq_rm(pCtx, seq_index, blocks_pos, -1);
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

int32_t ContextSequence::RemoveAndShift(const llama_vocab* pVocab, std::vector<ContextBlock>::iterator itBegin, std::vector<ContextBlock>::iterator itEnd)
{
	llm_util::dump_context(batch, pVocab, "prompt-full.txt");

	// Remove
	int32_t shift_amount = 0;
	for (auto it = itBegin; it != itEnd; ++it)
		shift_amount += (*it).length();
	if (shift_amount == 0)
		return 0;

	int32_t n_used = llama_kv_self_used_cells(pCtx);
	int32_t pos_remove_begin = (*itBegin).offset;
	int32_t pos_remove_end = pos_remove_begin + shift_amount;

	blocks.erase(itBegin, itEnd);
	AssignBlockPositions();

	if (!llama_kv_self_seq_rm(pCtx, seq_index, pos_remove_begin, pos_remove_end))
		return 0;

	// Shift
	llama_kv_self_seq_add(pCtx, seq_index, pos_remove_end, cursor_pos, -shift_amount);
	llama_kv_self_update(pCtx);
	
	int32_t n_used_after = llama_kv_self_used_cells(pCtx);
	assert(n_used_after < n_used);

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

	llm_util::dump_context(batch, pVocab, "prompt-full.txt");

	return (int32_t)-shift_amount;
}


int32_t ContextSequence::EraseTokens(int32_t from, int32_t length)
{
	if (from < 0 || from >= Constants::Context::Size || length <= 0  || from + length > Constants::Context::Size)
		return 0;

	llama_kv_self_seq_rm(pCtx, seq_index, from, from + length);

	// Update batch
	for (int32_t i = 0; i < length; ++i)
	{
		int32_t idx = from + i;
		batch.pos[idx] = 0;
		batch.token[idx] = 0;
		batch.n_seq_id[idx] = 0;
		batch.logits[idx] = false;
	}
	batch.n_tokens -= length;
	return length;
}

int32_t ContextSequence::BatchRemove(int32_t begin, int32_t end)
{
	int32_t n_removed = end - begin;

	// Remove
	llama_kv_self_seq_rm(pCtx, seq_index, begin, end);
	llama_kv_self_seq_add(pCtx, seq_index, end, -1, -n_removed);
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
	llama_kv_self_seq_add(pCtx, seq_index, pos, -1, length);
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
	llama_kv_self_seq_add(pCtx, seq_index, pos, -1, offset);
	return offset;
}

int32_t ContextSequence::BatchWrite(const std::vector<llama_token>& tokens, int32_t pos)
{
	// Add to context batch
	int32_t n_tokens = (int32_t)tokens.size();
	for (int32_t i = 0; i < n_tokens; ++i)
	{
		int idx = pos + i;
		batch.token[idx] = tokens[i];
		batch.pos[idx] = idx;
		batch.n_seq_id[idx] = 1;
		batch.seq_id[idx][0] = 0;
		batch.logits[i] = false;
	}
	return n_tokens;
}

void ContextSequence::BatchSetSequences(int32_t pos, const std::vector<int32_t>& seqIds)
{
	batch.n_seq_id[pos] = toI(seqIds.size());
	for (size_t i = 0; i < seqIds.size() && i < Constants::Context::MaxSequences; ++i)
		batch.seq_id[pos][i] = seqIds[i];
}

int32_t ContextState::get_max_position() const
{
	int32_t max = 0;
	for (auto& seq : sequences)
	{
		int32_t offset = 0;
		for (auto& block : seq.blocks)
		{
			if (block.is_static() || block.is_cached())
			{
				offset += block.length();
				continue;
			}
			break;
		}
		max = std::max(max, offset);
	}
	return max;
}