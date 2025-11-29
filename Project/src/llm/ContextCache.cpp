#include "llm/ContextCache.h"
#include "llm/LLMUtility.h"
#include "Constants.h"
#include <cassert>

ContextCache::ContextCache(int32_t max_size, int32_t n_seq_max) : 
	_max_size { max_size },
	_n_seq_max { n_seq_max }
{
	_batch = std::make_unique<llama_batch>(llm_util::init_batch(max_size, n_seq_max));
}

ContextCache::~ContextCache()
{
	if (_batch)
	{
		llm_util::free_batch(*_batch.get());
		_batch.reset();
	}
}

int32_t ContextCache::BatchWrite(std::span<llama_token> tokens, SequenceId seq_id, int32_t pos)
{
	// Add to context batch
	auto seq_ids = llm_util::get_sequence_indices(seq_id, _n_seq_max);
	int32_t n_seq = toI(seq_ids.size());
	int32_t n_tokens = toI(tokens.size());
	llama_batch& batch = *_batch.get();

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

int32_t ContextCache::BatchRemove(int32_t begin, int32_t end)
{
	llama_batch& batch = *_batch.get();
	int32_t n_removed = Shift(end, batch.n_tokens - end, -(end - begin)) * -1;
	batch.n_tokens -= n_removed;
	return n_removed;
}

int32_t ContextCache::BatchClear(int32_t begin, int32_t end)
{
	// Update batch
	llama_batch& batch = *_batch.get();
	for (int32_t i = begin; i < end; ++i)
	{
		batch.pos[i] = 0;
		batch.token[i] = 0;
		batch.n_seq_id[i] = 0;
		for (int32_t itSeq = 0; itSeq < _n_seq_max; ++itSeq)
			batch.seq_id[i][itSeq] = -1;
		batch.logits[i] = false;
	}
	batch.n_tokens = begin;
	return std::max(end - begin, 0);
}

int32_t ContextCache::BatchAllocate(int32_t pos, int32_t length)
{
	// Remove
//	llama_kv_self_seq_add(pCtx, 0, pos, -1, length);
//	llama_kv_self_update(pCtx);
//	assert(false && "This doesn't handle sequences yet.");

//	int32_t ctx_size = llama_n_ctx(pCtx);

	// Update batch
	llama_batch& batch = *_batch.get();
	int32_t n_batch = batch.n_tokens;
	for (int32_t i = 0; i < n_batch - pos; ++i)
	{
		if (i >= _max_size)
			continue;

		int32_t idx = n_batch + length - i - 1;
		batch.pos[idx] = idx;
		batch.token[idx] = batch.token[idx - length];
		batch.n_seq_id[idx] = batch.n_seq_id[idx - length];
		for (int32_t itSeq = 0; itSeq < _n_seq_max; ++itSeq)
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
		for (int32_t itSeq = 0; itSeq < _n_seq_max; ++itSeq)
			batch.seq_id[idx][itSeq] = -1;
		batch.logits[idx] = false;
	}

	return length;
}

int32_t ContextCache::Shift(int32_t pos, int32_t len, int32_t offset)
{
	// Shift down
	if (len < 0)
		len = _max_size - pos;

	int32_t src_pos = pos;
	int32_t dest_pos = pos + offset;
	llama_batch& batch = *_batch.get();
	if (src_pos > dest_pos) // Shifting up, write top down
	{
		for (int32_t i = 0; i < offset; ++i)
		{
			int idx = dest_pos + i;
			if (idx < 0 || idx >= _max_size)
				continue;

			batch.pos[idx] = idx;
			batch.token[idx] = batch.token[src_pos + i];
			batch.n_seq_id[idx] = batch.n_seq_id[src_pos + i];
			for (int32_t itSeq = 0; itSeq < _n_seq_max; ++itSeq)
				batch.seq_id[idx][itSeq] = batch.seq_id[src_pos + i][itSeq];
			batch.logits[idx] = batch.logits[src_pos + i];
		}
	}
	else if (src_pos < dest_pos) // Shifting down, write bottom up
	{
		for (int32_t i = 0; i < offset; ++i)
		{
			int idx = dest_pos + len - i - 1;
			if (idx < 0 || idx >= _max_size)
				continue;

			batch.pos[idx] = idx;
			batch.token[idx] = batch.token[src_pos + len - i - 1];
			batch.n_seq_id[idx] = batch.n_seq_id[src_pos + len - i - 1];
			for (int32_t itSeq = 0; itSeq < _n_seq_max; ++itSeq)
				batch.seq_id[idx][itSeq] = batch.seq_id[src_pos + len - i - 1][itSeq];
			batch.logits[idx] = batch.logits[src_pos + len - i - 1];
		}		
	}

	return offset;
}


void ContextCache::BatchSetSequences(int32_t pos, const std::vector<int32_t>& seqIds)
{
	llama_batch& batch = *_batch.get();
	batch.n_seq_id[pos] = toI(seqIds.size());
	for (size_t i = 0; i < seqIds.size() && i < _n_seq_max; ++i)
		batch.seq_id[pos][i] = seqIds[i];
}

void ContextCache::BatchSetSequences(int32_t from, int32_t length, SequenceId seq_id)
{
	auto seqIds = llm_util::get_sequence_indices(seq_id, _n_seq_max);
	int32_t n_seq = toI(seqIds.size());
	int32_t to = from + length;
	llama_batch& batch = *_batch.get();
	for (int32_t pos = from; pos < to; ++pos)
	{
		batch.n_seq_id[pos] = n_seq;
		for (size_t i = 0; i < seqIds.size() && i < _n_seq_max; ++i)
			batch.seq_id[pos][i] = seqIds[i];
	}
}

llama_batch ContextCache::CreateBatchView(int32_t pos, int32_t length) const
{
	llama_batch& batch = *_batch.get();
	return llm_util::create_batch_view(batch, pos, length);
}

void ContextCache::CopyTokens(int32_t from, int32_t to)
{
	llama_batch& batch = *_batch.get();
	batch.token[to] = batch.token[from];
	batch.n_seq_id[to] = batch.n_seq_id[from];
	for (int32_t itSeq = 0; itSeq < _n_seq_max; ++itSeq)
		batch.seq_id[to][itSeq] = batch.seq_id[from][itSeq];
	batch.logits[to] = batch.logits[from];
}
