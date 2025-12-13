#include "llm/Context.h"
#include "llm/LLMUtility.h"
#include "util/Common.h"
#include "Constants.h"
#include <cassert>
#include <algorithm>
#include <format>

using namespace common_util;

Context::Context(const ModelState& model)
{
	_pModel = &model;
	_cache = std::make_shared<ContextCache>(model.ctx_size, model.num_sequences);
	_pCtx = model.pCtx;
	_pVocab = model.pVocab;
	_num_sequences = model.num_sequences;
}

int32_t Context::GetUsedKVCacheCells() const
{
	return llama_kv_self_used_cells(_pModel->pCtx);
}

bool Context::ReserveTokens(int32_t ctx_reserve, bool bForce)
{
	int32_t n_ctx_used = GetUsedKVCacheCells();
	int32_t ctx_size = _pModel->ctx_size;
	if (n_ctx_used + ctx_reserve >= ctx_size || bForce)
	{
		DebugPrintLn(">> REALLOCATING CONTEXT");
		int32_t shift = AllocateKVCache(ctx_reserve);
		DebugPrintLn(std::format(">> Allocated {} tokens.", std::abs(shift), ctx_size - cursor_pos));
		assert(ctx_size - cursor_pos >= Constants::Context::MaxResponseLength);
		return true;
	}
	return false;
}

void Context::Initialize()
{
	llama_kv_self_clear(_pCtx);
	_blocks.clear();
	_cache->Clear();
}

void Context::RefreshBlockPositions()
{
	int32_t offset = 0;
	for (auto& block : _blocks)
	{
		if (offset >= chat_begin_pos)
			block.offset = offset;
		if (block.flags.IsSet(ContextBlockFlag::Static))
			offset = std::max(offset, block.offset + block.length());
		else
			offset += block.length();
	}
}

std::optional<int32_t> Context::DecodeTokens(const std::vector<llama_token>& tokens, int32_t pos, SequenceId seq_id)
{
	if (tokens.size() == 0)
		return std::make_optional(0);

	int32_t n_tokens = static_cast<int32_t>(tokens.size());
	auto seq_indices = llm_util::get_sequence_indices(seq_id, _num_sequences);
	auto [batch_ref, batch_n] = GetCache().GetBatch();
	llama_batch& batch = batch_ref.get();
	
	GetCache().BatchWrite(std::span(tokens.begin(), tokens.end()), seq_id, pos);

	// Decode
	llama_batch batch_view = llm_util::create_batch_view(batch, pos, n_tokens);
	if (batch_view.n_tokens > 0 && llama_decode(_pCtx, batch_view) != 0)
		return std::nullopt; // Error


//	cursor_pos += n_tokens;
	return std::make_optional(n_tokens);
}

int32_t Context::DecodeUncached(int32_t cursor_pos)
{
	for (auto& block : _blocks)
	{
		if (block.flags.IsSet(ContextBlockFlag::Cached))
			continue;

		std::vector<llama_seq_id> seqs = block.get_sequence_ids(_num_sequences);
		llama_batch batch_view = llm_util::create_batch(block.tokens, seqs, _num_sequences, block.offset);
		if (!llama_decode(_pCtx, batch_view))
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

bool Context::RebuildKVCache()
{
	auto& cache = *_cache.get();

	int r;
	if constexpr (::Disabled)
	{
		// Clear everything
		llama_kv_self_clear(_pCtx);
		auto batch_view = cache.GetBatchView(0, cache.length());
		r = llama_decode(_pCtx, batch_view);
	}
	else
	{
		// Clear only non-static blocks
		int32_t blocks_pos = chat_begin_pos;

		for (size_t i = 0; i < Constants::Context::AllSequenceIDs.size(); ++i)
			llama_kv_self_seq_rm(_pCtx, (int32_t)i, blocks_pos, -1);

		auto batch_view = cache.GetBatchView(blocks_pos, cache.length() - blocks_pos);
		r = llama_decode(_pCtx, batch_view);
	}

	cursor_pos = cache.length();
	return r == 0;
}

int32_t Context::RemoveBlock(const ContextBlock& block, bool bShift)
{
	assert(block.sequenceId != SequenceId::None);

	auto iter_block = std::find_if(_blocks.cbegin(), _blocks.cend(), [&block](const ContextBlock& b) { return &block == &b; });
	return RemoveBlocks(iter_block, iter_block + 1_uz, bShift);
}

int32_t Context::RemoveBlocks(std::vector<ContextBlock>::const_iterator begin, std::vector<ContextBlock>::const_iterator end, bool bShift)
{
	int32_t idx_from = toI(std::distance(_blocks.cbegin(), begin));
	int32_t idx_to = toI(std::distance(_blocks.cbegin(), end)) - 1;

#if _DEBUG
	int32_t n_used = llama_kv_self_used_cells(_pCtx);
#endif
	int32_t total_length = 0;

	// Remove tokens
	for (int32_t i = idx_to; i >= idx_from; --i)
	{
		auto& block = _blocks[i];
		assert(block.sequenceId != SequenceId::None);

		if (!llama_kv_self_seq_rm(_pCtx, -1, block.offset, block.offset + block.length()))
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
				llama_kv_self_seq_add(_pCtx, seq_id, _blocks[i].offset, _blocks[i].offset + _blocks[i].length(), -total_length);
			}
			_blocks[i].offset -= total_length;
		}

		// Update batch
		auto& cache = *_cache.get();
		cache.BatchRemove(pos_remove_begin, pos_remove_end);
	}

	llama_kv_self_defrag(_pCtx);

#if _DEBUG
	llama_kv_self_update(_pCtx);
	int32_t n_used_after = llama_kv_self_used_cells(_pCtx);
	assert(n_used_after < n_used);
#endif

	return (int32_t)-total_length;
}

void Context::ClearTokensBelow(int32_t pos)
{
	llm_util::erase_bottom(_pCtx, _num_sequences, pos);
	GetCache().ClearTokensFrom(pos);
}

int32_t Context::AllocateKVCache(int32_t min_reserve)
{
	// Allocate and shift context window
	int n_ctx_used = llama_kv_self_used_cells(_pCtx);
	int32_t ctx_size = llama_n_ctx(_pCtx);

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

int32_t Context::DecrementTTL(int32_t time)
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

int32_t Context::GetBlockAppendOffset() const
{
	int32_t offset = 0;
	for (auto& block : _blocks)
		offset = std::max(offset, block.offset + block.length());
	return offset;
}

int32_t Context::EraseChat()
{
	auto itFirst = std::find_if(_blocks.begin(), _blocks.end(), [](const ContextBlock& block) { return !block.flags.IsSet(ContextBlockFlag::Static); });
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
		llm_util::erase_bottom(_pCtx, _num_sequences, block_pos);
		auto& cache = *_cache.get();
		cache.ClearRange(block_pos, cache.length());

		cursor_pos = block_pos;
		chat_begin_pos = block_pos;
		return block_pos;
	}
}

void Context::AppendBlock(const ContextBlock& block)
{
	_blocks.push_back(block);
}

void Context::AppendBlock(ContextBlock&& block)
{
	_blocks.emplace_back(std::move(block));
}

void Context::EraseVolatile()
{
	std::erase_if(_blocks, [](const ContextBlock& block) { return block.is_volatile(); });
}