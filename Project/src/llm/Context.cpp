#include "llm/Context.h"
#include "llm/LLMUtility.h"
#include <cassert>

int32_t ContextSequence::AssignBlockPositions()
{
	int32_t offset = 0;
	for (auto& block : blocks)
	{
		block.offset = offset;
		offset += block.length();
	}
	return offset;
}

bool ContextSequence::RebuildKVCache()
{
#if FALSE // Entire context
	llama_kv_self_clear(pCtx);
	int r = llama_decode(pCtx, batch);
#else // Post system_prompt
	llama_kv_self_seq_rm(pCtx, seq_index, blocks_pos, -1);
	auto batch_view = llm_util::create_batch_view(batch, blocks_pos, batch.n_tokens - blocks_pos);
	int r = llama_decode(pCtx, batch_view);
#endif

	current_pos = batch.n_tokens;
	return r == 0;
}


int32_t ContextSequence::RemoveAndShift(const llama_vocab* pVocab, std::vector<ContextBlock>::iterator itBegin, std::vector<ContextBlock>::iterator itEnd)
{
//	dump_context(seq.batch, pVocab, "prompt-full.txt");

	// Remove
	int32_t shift_amount = 0;
	for (auto it = itBegin; it != itEnd; ++it)
		shift_amount += (*it).length();
	if (shift_amount == 0)
		return 0;

	int32_t n_used = llama_kv_self_used_cells(pCtx);

	int32_t pos_remove_begin = blocks_pos + (*itBegin).offset;
	int32_t pos_remove_end = pos_remove_begin + shift_amount;
	if (!llama_kv_self_seq_rm(pCtx, 0, pos_remove_begin, pos_remove_end))
		return 0;
	
	int32_t n_used_after = llama_kv_self_used_cells(pCtx);
	assert(n_used_after < n_used);

	// Shift
	llama_kv_self_seq_add(pCtx, 0, pos_remove_end, current_pos, -shift_amount);
	llama_kv_self_update(pCtx);

	// Update batch
	int32_t n_batch = batch.n_tokens;
	for (int32_t i = 0; i < n_batch - pos_remove_end; ++i)
	{
		batch.pos[pos_remove_begin + i] = pos_remove_begin + i;
		batch.token[pos_remove_begin + i] = batch.token[pos_remove_end + i];
		batch.n_seq_id[pos_remove_begin + i] = batch.n_seq_id[pos_remove_end + i];
		batch.seq_id[pos_remove_begin + i][0] = batch.seq_id[pos_remove_end + i][0];
		batch.logits[i] = false;
	}
	batch.n_tokens -= shift_amount;

//	dump_context(ctxState.batch, pVocab, "prompt-full.txt");

	return (int32_t)-shift_amount;
}