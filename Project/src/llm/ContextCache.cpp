#include <pch.h>
#include "llm/ContextBlock.h"
#include "llm/ContextCache.h"
#include "llm/LLMUtility.h"
#include "llm/LlamaApi.h"
#include <cassert>
#include <span>
#include <ranges>

namespace fig::llm
{
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

	int32_t ContextCache::BatchAddSingle(Token token, Sequences seq_ids, int32_t attn_pos, bool logits)
	{
		Batch& batch = *_batch.get();

		int idx = batch.n_tokens;
		batch.token[idx] = token;
		batch.pos[idx] = attn_pos;

		batch.n_seq_id[idx] = toI(seq_ids.size());
		for (size_t itSeq = 0; itSeq < seq_ids.size(); ++itSeq)
			batch.seq_id[idx][itSeq] = seq_ids[itSeq];
		batch.logits[idx] = logits;

		_length = ++batch.n_tokens;
		return 1;
	}

	std::pair<int32_t, int32_t> ContextCache::BatchWrite(std::span<const Token> tokens, SequenceSlots seq_id, int32_t cache_pos, ContextCursor& cursor)
	{
		auto r = BatchWrite(tokens, seq_id, cache_pos, cursor.as_int());
		cursor.increment(r.second);
		return r;
	}

	std::pair<int32_t, int32_t> ContextCache::BatchWrite(std::span<const Token> tokens, SequenceSlots seq_id, int32_t cache_pos, int32_t attn_pos)
	{
		// Add to context batch
		auto seq_ids = get_sequence_indices(seq_id, _n_seq_max);
		int32_t n_seq = toI(seq_ids.size());
		Batch& batch = *_batch.get();

		if (cache_pos < 0)
			cache_pos = batch.n_tokens;
		int32_t n_tokens = toI(tokens.size());

		for (int32_t i = 0; i < n_tokens; ++i)
		{
			int32_t idx = cache_pos + i;
			assert(idx < _max_size);

			batch.token[idx] = tokens[i];
			batch.pos[idx] = attn_pos + i;
			batch.n_seq_id[idx] = n_seq;
			for (size_t itSeq = 0; itSeq < seq_ids.size(); ++itSeq)
				batch.seq_id[idx][itSeq] = seq_ids[itSeq];
			batch.logits[idx] = false;
		}

		_length = batch.n_tokens += n_tokens;
		return std::make_pair(cache_pos, n_tokens); // <pos, len>
	}

	// Returns number of cells after removal
	int32_t ContextCache::RemoveBlock(const ContextBlock& block)
	{
		int32_t n_empty = 0;
		Batch& batch = *_batch.get();
		int32_t length = block.length();
		auto seq_ids = block.get_sequence_ids(_n_seq_max);
		for (int32_t i = 0; i < length; ++i)
		{
			int idx = block.cache_position + i;

			assert(block.tokens[i] == batch.token[idx]);

			if (block.sequenceSlots == SequenceSlot::Shared)
			{
				for (int32_t itSeq = 0; itSeq < _n_seq_max; ++itSeq)
					batch.seq_id[idx][itSeq] = -1;
				batch.n_seq_id[idx] = 0;
			}
			else
			{
				for (auto seq_id : seq_ids)
				{
					int32_t offset = 0;
					for (int32_t itSeq = 0; itSeq < _n_seq_max && batch.n_seq_id[idx]; ++itSeq)
					{
						if (batch.seq_id[idx][itSeq] == seq_id)
						{
							batch.seq_id[idx][itSeq] = -1;
							batch.n_seq_id[idx]--;
							break;
						}
					}
				}
			}

			if (batch.n_seq_id[idx] == 0)
			{
				// Clear
				batch.pos[idx] = 0;
				batch.token[idx] = 0;
				batch.logits[idx] = false;
				++n_empty;
			}
			else
			{
				auto seq_span = std::span(&batch.seq_id[idx][0], &batch.seq_id[idx][_n_seq_max - 1]);
				std::ranges::sort(seq_span, [](Sequence a, Sequence b) { return a > b; });
			}
		}
		return n_empty;
	}

	void ContextCache::ClearToken(int32_t pos)
	{
		// Update batch
		Batch& batch = *_batch.get();

		// Init src
		batch.pos[pos] = 0;
		batch.token[pos] = 0;
		batch.n_seq_id[pos] = 0;
		for (int32_t itSeq = 0; itSeq < _n_seq_max; ++itSeq)
			batch.seq_id[pos][itSeq] = -1;
		batch.logits[pos] = false;
	}

	void ContextCache::ClearTokens(int32_t index, int32_t length)
	{
		// Update batch
		Batch& batch = *_batch.get();
		for (int32_t idx = index; idx < index + length && idx < _max_size; ++idx)
		{
			batch.pos[idx] = 0;
			batch.token[idx] = 0;
			batch.n_seq_id[idx] = 0;
			for (int32_t itSeq = 0; itSeq < _n_seq_max; ++itSeq)
				batch.seq_id[idx][itSeq] = -1;
			batch.logits[idx] = false;
		}
	}

	void ContextCache::ClearTokensFromIndex(int32_t index)
	{
		ClearTokens(index, _max_size);

		Batch& batch = *_batch.get();
		_length = batch.n_tokens = index;
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

	void ContextCache::MoveBlock(ContextBlock& block, int32_t offset)
	{
		int32_t src_pos = block.cache_position;
		CopyTokens(src_pos, src_pos + block.length(), offset);
		block.cache_position += offset;
	}

	void ContextCache::ShiftBlock(ContextBlock& block, int32_t offset)
	{
		int32_t src_pos = block.cache_position;
		ShiftTokens(src_pos, src_pos + block.length(), offset);
		block.attn_position += offset;
	}

	void ContextCache::CopyTokens(int32_t begin, int32_t end, int32_t offset)
	{
		Batch& batch = *_batch.get();

		int32_t src_pos = begin;
		int32_t dest_pos = begin + offset;
		int32_t length = std::abs(begin - end);
		if (src_pos > dest_pos)
		{
			// Copy tokens up
			for (int32_t i = 0; i < length; ++i)
			{
				int src_idx = src_pos + i;
				int dest_idx = dest_pos + i;
				assert(dest_idx >= 0 && dest_idx < _max_size && src_idx >= 0 || src_idx < _max_size);

				batch.pos[dest_idx] = batch.pos[src_idx];
				batch.token[dest_idx] = batch.token[src_idx];
				batch.n_seq_id[dest_idx] = batch.n_seq_id[src_idx];
				for (int32_t itSeq = 0; itSeq < _n_seq_max; ++itSeq)
					batch.seq_id[dest_idx][itSeq] = batch.seq_id[src_idx][itSeq];
				batch.logits[dest_idx] = batch.logits[src_idx];

				ClearToken(src_idx);
			}
		}
		else if (src_pos < dest_pos)
		{
			// Copy tokens down
			for (int32_t i = 0; i < length; ++i)
			{
				int src_idx = src_pos + length - i - 1;
				int dest_idx = dest_pos + length - i - 1;
				assert(src_idx >= 0 || src_idx < _max_size && dest_idx >= 0 && dest_idx < _max_size);

				batch.pos[dest_idx] = batch.pos[src_idx];
				batch.token[dest_idx] = batch.token[src_idx];
				batch.n_seq_id[dest_idx] = batch.n_seq_id[src_idx];
				for (int32_t itSeq = 0; itSeq < _n_seq_max; ++itSeq)
					batch.seq_id[dest_idx][itSeq] = batch.seq_id[src_idx][itSeq];
				batch.logits[dest_idx] = batch.logits[src_idx];

				ClearToken(src_idx);
			}
		}
	}

	void ContextCache::ShiftTokens(int32_t begin, int32_t end, int32_t offset)
	{
		Batch& batch = *_batch.get();
		int32_t length = std::abs(begin - end);
		for (int32_t i = 0; i < length; ++i)
			batch.pos[begin + i] += offset;
	}

	void ContextCache::AdjustLength(int32_t offset)
	{
		Batch& batch = *_batch.get();
		_length = batch.n_tokens += offset;
	}

	void ContextCache::BatchSetSequences(int32_t pos, const std::vector<int32_t>& seqIds)
	{
		Batch& batch = *_batch.get();
		batch.n_seq_id[pos] = toI(seqIds.size());
		for (size_t i = 0; i < seqIds.size() && i < _n_seq_max; ++i)
			batch.seq_id[pos][i] = seqIds[i];
	}

	void ContextCache::BatchSetSequences(int32_t begin, int32_t end, SequenceSlots seq_id)
	{
		Sequences seqIds = get_sequence_indices(seq_id, _n_seq_max);
		int32_t n_seq = toI(seqIds.size());
		int32_t length = (end - begin);

		Batch& batch = *_batch.get();
		for (int32_t idx = begin; idx < end; ++idx)
		{
			batch.n_seq_id[idx] = n_seq;
			for (size_t itSeq = 0; itSeq < seqIds.size() && itSeq < _n_seq_max; ++itSeq)
				batch.seq_id[idx][itSeq] = seqIds[itSeq];
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

	Batch ContextCache::GetView(int32_t pos, int32_t length) const
	{
		Batch& batch = *_batch.get();
		return llama::create_batch_view(batch, pos, length);
	}
}