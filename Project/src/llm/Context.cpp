#include "llm/Context.h"
#include "llm/LLMUtility.h"
#include "util/Common.h"
#include "Constants.h"
#include <cassert>
#include <algorithm>
#include <format>
#include <cassert>

llama_seq_id ContextBlock::get_any_sequence_id() const noexcept
{
	for (size_t i = 0; i < Constants::Context::AllSequenceIDs.size() && i < Constants::Context::MaxSequences; ++i)
	{
		if ((bool)(this->sequenceId & Constants::Context::AllSequenceIDs[i]))
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
		if ((bool)(this->sequenceId & Constants::Context::AllSequenceIDs[i]))
			seqIds.push_back(toI(i));
	}
	return seqIds;
}

ContextState::ContextState(const ModelState& model, int32_t n_seq)
{
	pCtx = model.pCtx;
	pVocab = model.pVocab;
	num_sequences = n_seq;
	max_tokens = llama_n_ctx(pCtx);
	
	sequence = ContextSequence(pCtx, n_seq);
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

void ContextState::Initialize()
{
	llama_kv_self_clear(pCtx);
}

ContextSequence::ContextSequence(llama_context* pCtx, int32_t n_seq_max)
{
	this->pCtx = pCtx;
	this->n_seq_max = n_seq_max;

	int32_t ctx_size = llama_n_ctx(pCtx);
	_cache = std::make_shared<ContextCache>(ctx_size, n_seq_max);
}

int32_t ContextSequence::get_max_position() const
{
	int32_t max = 0;
	for (auto& block : _blocks)
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

void ContextSequence::RefreshBlockPositions()
{
	int32_t offset = 0;
	for (auto& block : _blocks)
	{
		if (offset >= chat_begin_pos)
			block.offset = offset;
		if (CheckEnumFlag(block.flags, ContextBlockFlag::Static))
			offset = std::max(offset, block.offset + block.length());
		else
			offset += block.length();
	}
}

std::pair<int32_t, bool> ContextSequence::DecodeTokens(const std::vector<llama_token>& tokens, int32_t pos, SequenceId seq_id)
{
	if (tokens.size() == 0)
		return std::make_pair(0, true);

	int32_t n_tokens = static_cast<int32_t>(tokens.size());
	auto seq_indices = llm_util::get_sequence_indices(seq_id, n_seq_max);
	auto [batch_ref, batch_n] = GetCache().GetBatch();
	llama_batch& batch = batch_ref.get();
	
	GetCache().BatchWrite(std::span(tokens.begin(), tokens.end()), seq_id, pos);

	// Decode
	llama_batch batch_view = llm_util::create_batch_view(batch, pos, n_tokens);
	if (batch_view.n_tokens > 0 && llama_decode(pCtx, batch_view) != 0)
		return std::make_pair(0, false); // Error


//	cursor_pos += n_tokens;
	return std::make_pair(n_tokens, true);
}

int32_t ContextSequence::DecodeUncached(int32_t cursor_pos)
{
	for (auto& block : _blocks)
	{
		if (CheckEnumFlag(block.flags, ContextBlockFlag::Cached))
			continue;

		std::vector<llama_seq_id> seqs = block.get_sequence_ids(n_seq_max);
		llama_batch batch_view = llm_util::create_batch(block.tokens, seqs, n_seq_max, block.offset);
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

	auto& cache = *_cache.get();
	auto batch_view = cache.CreateBatchView(blocks_pos, cache.length() - blocks_pos);
	int r = llama_decode(pCtx, batch_view);
#endif

	cursor_pos = cache.length();
	return r == 0;
}

int32_t ContextSequence::RemoveBlock(const ContextBlock& block, bool bShift)
{
	assert(block.sequenceId != SequenceId::None);

	auto iter_block = std::find_if(_blocks.cbegin(), _blocks.cend(), [&block](const ContextBlock& b) { return &block == &b; });
	return RemoveBlocks(iter_block, iter_block + 1_sz, bShift);
}

int32_t ContextSequence::RemoveBlocks(std::vector<ContextBlock>::const_iterator begin, std::vector<ContextBlock>::const_iterator end, bool bShift)
{
	int32_t idx_from = toI(std::distance(_blocks.cbegin(), begin));
	int32_t idx_to = toI(std::distance(_blocks.cbegin(), end)) - 1;

#if _DEBUG
	int32_t n_used = llama_kv_self_used_cells(pCtx);
#endif
	int32_t total_length = 0;

	// Remove tokens
	for (int32_t i = idx_to; i >= idx_from; --i)
	{
		auto& block = _blocks[i];
		assert(block.sequenceId != SequenceId::None);

		if (!llama_kv_self_seq_rm(pCtx, -1, block.offset, block.offset + block.length()))
			return 0;
		
		total_length += block.length();
		_blocks.erase(_blocks.begin() + i);
	}

	if (bShift)
	{
		int32_t pos_remove_begin = _blocks[idx_from].offset;
		int32_t pos_remove_end = pos_remove_begin + total_length;

		// Shift up (block-wise because of sequences)
		for (size_t i = _blocks.size() - 1; i >= idx_from; --i)
		{
			if (_blocks[i].is_cached())
			{
				int32_t seq_id = _blocks[i].get_any_sequence_id();
				llama_kv_self_seq_add(pCtx, seq_id, _blocks[i].offset, _blocks[i].offset + _blocks[i].length(), -total_length);
			}
			_blocks[i].offset -= total_length;
		}

		// Update batch
		auto& cache = *_cache.get();
		cache.BatchRemove(pos_remove_begin, pos_remove_end);
	}

	llama_kv_self_defrag(pCtx);

#if _DEBUG
	llama_kv_self_update(pCtx);
	int32_t n_used_after = llama_kv_self_used_cells(pCtx);
	assert(n_used_after < n_used);
#endif

	return (int32_t)-total_length;
}

void ContextSequence::ClearTokensBelow(int32_t pos)
{
	llm_util::erase_bottom(pCtx, n_seq_max, pos);
	GetCache().BatchClearFrom(pos);
}

int32_t ContextSequence::AllocateKVCache(int32_t min_reserve)
{
	// Allocate and shift context window
	int n_ctx_used = llama_kv_self_used_cells(pCtx);
	int32_t ctx_size = llama_n_ctx(pCtx);

	int32_t ctx_chat_max = ctx_size - chat_begin_pos; // Exclude system prompt
	int32_t free_tokens = std::max(min_reserve, toI(ctx_chat_max * (1.0f - Constants::Context::WindowKeepRatio)));
		
	auto itFirst = std::find_if(_blocks.begin(), _blocks.end(), [](auto& block) { return !block.is_static(); });
	assert(itFirst != _blocks.end());

	int32_t total = 0;
	auto itLast = itFirst;
	while (itLast != _blocks.end() && total < free_tokens && itLast->is_cached())
	{
		total += itLast->length();
		itLast++;
	}

	assert(itFirst != itLast);

	if (itFirst != itLast)
	{
		int32_t shift = RemoveBlocks(itFirst, itLast);

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
	for (int32_t i = (int32_t)_blocks.size() - 1; i >= 0; --i)
	{
		auto& block = _blocks[i];
		if (!block.is_temporary())
			continue;

		block.ttl -= time;
		if (block.ttl > 0)
			continue;
		
		if (block.is_cached())
		{
			// Remove from context
			int32_t shift = RemoveBlock(block);
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
	for (auto& block : _blocks)
		offset = std::max(offset, block.offset + block.length());
	return offset;
}

int32_t ContextSequence::EraseChat()
{
	auto itFirst = std::find_if(_blocks.begin(), _blocks.end(), [](const ContextBlock& block) { return !CheckEnumFlag(block.flags, ContextBlockFlag::Static); });
	if (itFirst == _blocks.end())
	{
		int32_t block_pos = GetBlockAppendOffset();
		cursor_pos = block_pos;
		chat_begin_pos = block_pos;
		return block_pos;
	}
	else
	{
		int32_t block_pos = itFirst->offset;
		llm_util::erase_bottom(pCtx, n_seq_max, block_pos);
		auto& cache = *_cache.get();
		cache.BatchClear(block_pos, cache.length());

		cursor_pos = block_pos;
		chat_begin_pos = block_pos;
		return block_pos;
	}
}

void ContextSequence::AppendBlock(const ContextBlock& block)
{
	_blocks.push_back(block);
}

void ContextSequence::AppendBlock(ContextBlock&& block)
{
	_blocks.emplace_back(std::move(block));
}

void ContextSequence::EraseVolatile()
{
	std::erase_if(_blocks, [](const ContextBlock& block) { return block.is_volatile(); });
}