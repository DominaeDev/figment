#include <pch.h>
#include "llm/ContextCache.h"
#include "llm/LLMUtility.h"
#include "llm/LlamaApi.h"
#include "Constants.h"
#include <cassert>

using namespace fig::llm;

ContextCache::ContextCache(int32_t max_size, int32_t n_seq_max) : 
	_max_size { max_size },
	_n_seq_max { n_seq_max },
	_length { 0 }
{
	_batch = std::make_unique<Batch>(llama::init_batch(max_size, n_seq_max));
}

ContextCache::~ContextCache()
{
	if (_batch)
	{
		llama::free(*_batch.get());
		_batch.reset();
	}
}

void ContextCache::Clear()
{
	if (_batch)
		llama::free(*_batch.get());
	_batch = std::make_unique<Batch>(llama::init_batch(_max_size, _n_seq_max));
	_length = 0;
}

int32_t ContextCache::BatchAddSingle(Token token, SequenceIndices seq_ids, int32_t pos)
{
	int32_t n_seq = toI(seq_ids.size());

	Batch& batch = *_batch.get();
	int idx = pos;
	batch.token[idx] = token;
	batch.pos[idx] = idx;
	batch.n_seq_id[idx] = n_seq;
	for (size_t itSeq = 0; itSeq < seq_ids.size(); ++itSeq)
		batch.seq_id[idx][itSeq] = seq_ids[itSeq];
	batch.logits[idx] = true;

	++_length;
	batch.n_tokens = _length;
	return 1;
}

int32_t ContextCache::BatchWrite(std::span<const Token> tokens, SequenceId seq_id, int32_t pos)
{
	// Add to context batch
	auto seq_ids = fig::llm::utility::get_sequence_indices(seq_id, _n_seq_max);
	int32_t n_seq = toI(seq_ids.size());
	int32_t n_tokens = toI(tokens.size());
	Batch& batch = *_batch.get();

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

	_length = std::max(pos + n_tokens, _length);
	batch.n_tokens = _length;
	return n_tokens;
}

int32_t ContextCache::BatchRemove(int32_t begin, int32_t end)
{
	Batch& batch = *_batch.get();
	int32_t n_removed = ShiftTokens(end, _length - end, -(end - begin));
	_length += n_removed;
	batch.n_tokens = _length;
	return n_removed;
}

int32_t ContextCache::ClearRange(int32_t begin, int32_t end)
{
	// Update batch
	Batch& batch = *_batch.get();
	for (int32_t i = begin; i < end; ++i)
	{
		batch.pos[i] = 0;
		batch.token[i] = 0;
		batch.n_seq_id[i] = 0;
		for (int32_t itSeq = 0; itSeq < _n_seq_max; ++itSeq)
			batch.seq_id[i][itSeq] = -1;
		batch.logits[i] = false;
	}
//	_length = begin;
//	batch.n_tokens = _length;
	return std::max(end - begin, 0);
}

void ContextCache::ClearTokensFrom(int32_t pos)
{
	Batch& batch = *_batch.get();
	for (int i = 0; i < batch.n_tokens; ++i)
	{
		if (batch.pos[i] >= pos)
		{
			_length = i;
			batch.n_tokens = _length;
			break;
		}
	}
}

int32_t ContextCache::BatchAllocate(int32_t pos, int32_t length)
{
	// Remove
//	llama_kv_self_seq_add(pCtx, 0, pos, -1, length);
//	llama_kv_self_update(pCtx);
//	assert(false && "This doesn't handle sequences yet.");

//	int32_t ctx_size = llama_n_ctx(pCtx);

	// Update batch
	Batch& batch = *_batch.get();
	int32_t n_batch = _length;
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

	_length += length;
	batch.n_tokens = _length;
	return length;
}

int32_t ContextCache::ShiftTokens(int32_t pos, int32_t len, int32_t offset)
{
	// Shift down
	if (len < 0)
		len = _max_size - pos;

	int32_t src_pos = pos;
	int32_t dest_pos = pos + offset;
	Batch& batch = *_batch.get();
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
	Batch& batch = *_batch.get();
	batch.n_seq_id[pos] = toI(seqIds.size());
	for (size_t i = 0; i < seqIds.size() && i < _n_seq_max; ++i)
		batch.seq_id[pos][i] = seqIds[i];
}

void ContextCache::BatchSetSequences(int32_t from, int32_t length, SequenceId seq_id)
{
	auto seqIds = fig::llm::utility::get_sequence_indices(seq_id, _n_seq_max);
	int32_t n_seq = toI(seqIds.size());
	int32_t to = from + length;
	Batch& batch = *_batch.get();
	for (int32_t pos = from; pos < to; ++pos)
	{
		batch.n_seq_id[pos] = n_seq;
		for (size_t i = 0; i < seqIds.size() && i < _n_seq_max; ++i)
			batch.seq_id[pos][i] = seqIds[i];
	}
}

void ContextCache::InitLogits()
{
	if (_length <= 0)
		return;

	Batch& batch = *_batch.get();
	for (int i = 0; i < _length - 1; ++i)
		batch.logits[i] = false;
	batch.logits[_length - 1] = true;  // Only need logits for last token
}

Batch ContextCache::GetBatchView(int32_t pos, int32_t length) const
{
	Batch& batch = *_batch.get();
	return llama::create_batch_view(batch, pos, length);
}

void ContextCache::CopyTokens(int32_t from, int32_t to)
{
	Batch& batch = *_batch.get();
	batch.token[to] = batch.token[from];
	batch.n_seq_id[to] = batch.n_seq_id[from];
	for (int32_t itSeq = 0; itSeq < _n_seq_max; ++itSeq)
		batch.seq_id[to][itSeq] = batch.seq_id[from][itSeq];
	batch.logits[to] = batch.logits[from];
}
