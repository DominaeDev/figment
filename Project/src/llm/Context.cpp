#include <pch.h>
#include "llm/Context.h"
#include "llm/LlamaApi.h"
#include "llm/LLMUtility.h"
#include "llm/ModelState.h"
#include "util/Common.h"
#include "model/ChatSession.h"
#include "Constants.h"
#include <cassert>
#include <algorithm>
#include <format>

using namespace fig::common_util;
using namespace fig::llm;
using namespace fig::llm_util;
using namespace fig::data;

Context::Context(const ModelState& model, int32_t num_sequences)
{
	_pModel = &model;
	_cache = std::make_shared<ContextCache>(model.ctx_size, num_sequences);
	_pCtx = model.pCtx;
	_pVocab = model.pVocab;
	_num_sequences = num_sequences;
}

int32_t Context::GetUsedKVCacheCells() const
{
	return llama::ctx_used_cells(_pCtx);
}

int32_t Context::ReserveTokens(int32_t ctx_reserve, bool bForce)
{
	int32_t ctx_size = _pModel->ctx_size;
	int32_t n_ctx_used = GetUsedKVCacheCells();
	if (n_ctx_used + ctx_reserve >= ctx_size || bForce)
	{
		int32_t allocated = AllocateKVCache(ctx_reserve);
		LogLn(std::format(">> REALLOCATING CONTEXT: Allocated {} tokens.", allocated));
		return allocated;
	}
	return 0;
}

void Context::Initialize()
{
	llama::ctx_clear(_pCtx);
	_blocks.clear();
	_cache->Clear();
}

void Context::TokenizeUncached(ChatSession& session)
{
	// Tokenize uncached messages
	for (auto& block : _blocks)
	{
		if (block.is_static() || block.is_cached() || !block.tokens.empty())
			continue;

		fig::string content = block.content;

		if (block.is_continuation()) // Continue response
			content = apply_chat_template_prefix(block.role, content, block.name); //! name?
		else if (block.role == Role::System)
			content = apply_chat_template({ Message { block.role, content, block.name } }, false);
		else
		{
			fig::llm_util::complete_message(content);
			content = apply_chat_template({ Message { block.role, content, block.name } }, false);
		}
		content = session.ApplyNames(content, block.role); //! @move?
		block.tokens = llama::tokenize(_pVocab, content, false);

		block.attn_position = -1;
		block.cache_position = -1;
	}
}

ContextCursor Context::DecodeUncached()
{
	if (_blocks.empty())
	{
		cursor_pos = 0;
		token_pos = 0;
		return ContextCursor(0);
	}

	auto& cache = GetCache();
	int32_t last_position = -1;
	int32_t attn_position = 0;

	// Reserve space
	int32_t total_reserve = 0;
	for (auto const& block : _blocks)
	{
		if (not block.is_cached())
			total_reserve += block.length();
	}
	ReserveTokens(total_reserve, false);

	// Decode
	for (auto it = _blocks.begin(); it != _blocks.end(); ++it)
	{
		auto& block = *it;
		if (block.is_cached())
		{
			attn_position = std::max(attn_position, block.attn_position + block.length());
			continue;
		}

		assert(last_position <= attn_position);
		assert(block.length() > 0);

		if (block.attn_position < 0)
			block.attn_position = attn_position;
		last_position = block.attn_position;

		// Write block to cache
		auto [batch_pos, _] = cache.BatchWrite(block.tokens, block.sequenceSlots, -1, block.attn_position);
		block.cache_position = batch_pos;

		if (block.is_continuation())
			cache.InitLogits(); // Enable logits for last token

		// ... and decode it.
		Batch batch_view = cache.GetView(block.cache_position, block.length());
		if (llama::ctx_decode(_pCtx, batch_view) != llama::DecodeError::NoError)
			return ContextCursor::Invalid; // Error

		block.flags.Set(ContextBlockFlag::Cached);
		block.flags.Unset(ContextBlockFlag::Contination);
		cursor_pos = block.cache_position + block.length();
		attn_position = std::max(attn_position, block.attn_position + block.length());
	}
	return ContextCursor { attn_position };
}

std::optional<int32_t> Context::DecodeSingleUncached(ContextBlock& block)
{
	assert(block.length() > 0);
	if (block.attn_position == -1)
		return std::nullopt;

	// Write block to cache
	auto [cache_pos, length] = GetCache().BatchWrite(block.tokens, block.sequenceSlots, block.cache_position, block.attn_position);
	block.cache_position = cache_pos;

	// ... and decode it.
	Batch batch_view = GetCache().GetView(block.cache_position, block.length());
	if (llama::ctx_decode(_pCtx, batch_view) != llama::DecodeError::NoError)
		return std::nullopt; // Error

	block.flags.Set(ContextBlockFlag::Cached);
	return length; // Shift
}

std::optional<int32_t> Context::RealizeUncachedBlocks()
{
	int32_t tokens_added = 0;
#if _DEBUG
	int32_t n_used_before = llama::ctx_used_cells(_pCtx);
#endif

	auto& cache = GetCache();

	int32_t total_shift = 0;
	size_t index = 0;
	for (auto itBlock = _blocks.begin(); itBlock != _blocks.end(); ++itBlock, ++index)
	{
		auto& block = *itBlock;
		if (block.is_cached())
			continue;

		assert(block.length() > 0);
		if (block.attn_position == -1)
		{
			// Find positions
			int32_t attn_position = 0;
			int32_t cache_position = 0;

			for (auto it = _blocks.begin(); it < itBlock; ++it)
			{
				auto& block = *it;
				if (block.is_cached())
				{
					attn_position = std::max(attn_position, block.attn_position + block.length());
					cache_position = std::max(cache_position, block.cache_position + block.length());
				}
			}

			block.attn_position = attn_position;
			block.cache_position = cache_position;
		}

		int32_t shift = block.length();
		int32_t block_cache_pos = block.cache_position;
		int32_t block_attn_pos = block.attn_position;
		tokens_added += shift;
		total_shift += shift;

		// Shift down subsequent blocks
		if (shift != 0)
		{
			assert(shift > 0);
			for (int32_t shift_idx = toI(_blocks.size()) - 1; shift_idx >= index + 1; --shift_idx)
			{
				auto& shift_block = _blocks[shift_idx];
				if (!shift_block.is_cached())
					continue;

				if (shift_block.attn_position >= block_attn_pos)
				{
					llama::ctx_move(_pCtx, shift_block, shift);
					cache.ShiftBlock(shift_block, shift);
				}

				if (shift_block.cache_position >= block_cache_pos)
					cache.MoveBlock(shift_block, shift);
			}
			llama::ctx_update(_pCtx);
		}

		// Add to cache (after shift)
		if (not DecodeSingleUncached(block))
			continue; // Error
	}

	if (tokens_added == 0)
		return 0;

	LogLn(std::format("Insert {} tokens.", tokens_added));

#if _DEBUG
	int32_t n_used_after = llama::ctx_used_cells(_pCtx);
	assert(n_used_after == n_used_before + tokens_added);
#endif

	llama::ctx_defrag(_pCtx);

	cursor_pos.increment(total_shift);
	token_pos.increment(total_shift);
	return tokens_added;
}

bool Context::RebuildKVCache()
{
	int r;
	if constexpr (::Disabled)
	{
		// Clear everything
		llama::ctx_clear(_pCtx);
		auto batch_view = _cache->GetView(0, _cache->length());
		r = llama_decode(_pCtx, batch_view);
	}
	else
	{
		// Clear only non-static blocks
		int32_t blocks_pos = chat_begin_pos.as_int();
		llama::ctx_remove(_pCtx, blocks_pos);

		auto batch_view = _cache->GetView(blocks_pos, _cache->length() - blocks_pos);
		r = llama_decode(_pCtx, batch_view);
	}

	cursor_pos = _cache->length();
	token_pos = chat_begin_pos;
	return r == 0;
}

std::optional<int32_t> Context::RemoveDiscardedBlocks()
{
	if (std::count_if(_blocks.begin(), _blocks.end(), [](const ContextBlock& block) { return block.is_cached() && block.is_discarded(); }) == 0)
		return 0;

	int32_t tokens_removed = 0;
#if _DEBUG
	int32_t n_used_before = llama::ctx_used_cells(_pCtx);
#endif
	
	auto& cache = GetCache();

	llama::ctx_update(_pCtx);

	int32_t total_shift = 0;
	for (int32_t i = toI(_blocks.size()) - 1; i >= 0; --i)
	{
		size_t block_idx = toUZ(i);
		auto& block = _blocks[block_idx];
		if (not block.is_discarded())
			continue;

		if (not block.is_cached())
		{
			// Not cached. Just remove block.
			container_remove_at(_blocks, block_idx);
			continue;
		}

		// Remove from cache
		int32_t block_cache_pos = block.cache_position;
		int32_t block_attn_pos = block.attn_position;
		int32_t block_len = toI(block.length());
		if (not llama::ctx_remove(_pCtx, block))
			return std::nullopt; // Error

		// Remove from mirrored cache
		int32_t removed = cache.RemoveBlock(block);
		int32_t shift = -removed;
		tokens_removed += removed;
		total_shift += shift;
		container_remove_at(_blocks, block_idx);

		if (shift != 0)
		{
			assert(shift < 0); // hmm

			// Shift up subsequent blocks
			for (size_t shift_idx = i; shift_idx < _blocks.size(); ++shift_idx)
			{
				auto& shift_block = _blocks[shift_idx];
				if (!shift_block.is_cached())
					continue;

				// Shift attention
				if (shift_block.attn_position > block_attn_pos)
				{
					if constexpr (Debugging)
					{
						// Validate token positions
						assert(shift_block.attn_position >= 0);
						auto [batch_ref, _] = GetCache().GetBatch();
						auto& batch = batch_ref.get();
						for (size_t tok = 0; tok < shift_block.tokens.size(); ++tok)
							assert(shift_block.tokens[tok] == batch.token[shift_block.cache_position + tok]);
					}
					llama::ctx_move(_pCtx, shift_block, shift);
					cache.ShiftBlock(shift_block, shift);
				}

				// Shift cache pos
				if (shift_block.cache_position > block_cache_pos)
					cache.MoveBlock(shift_block, shift);
			}
			llama::ctx_update(_pCtx);
		}
	}

	if (tokens_removed == 0)
		return 0;

	LogLn(std::format("Removed {} tokens.", tokens_removed));

	llama::ctx_defrag(_pCtx);
	llama::ctx_update(_pCtx);

#if _DEBUG
	int32_t n_used_after = llama::ctx_used_cells(_pCtx);
	assert(n_used_after == n_used_before - tokens_removed);
#endif
	
	cache.ClearTokensFromIndex(cache.length() + total_shift);
	cursor_pos.increment(total_shift);
	token_pos.increment(total_shift);

	return tokens_removed;
}

bool Context::DiscardBlock(const ContextBlock& block)
{
	auto itFind = std::find(_blocks.begin(), _blocks.end(), block);
	if (itFind != _blocks.end())
	{
		itFind->Discard();
		return true;
	}
	return false;
}

void Context::ClearTokensBelow(int32_t pos)
{
	llama::ctx_remove(_pCtx, pos);

	for (auto& block : _blocks)
	{
		if (not block.is_cached())
			continue;
		
		if (block.attn_position >= pos)
		{
			block.flags.Unset(ContextBlockFlag::Cached);
			block.attn_position = -1;
			block.cache_position = -1;
		}
	}
	auto cache_pos = GetUncachedOffset();
	_cache->ClearTokensFromIndex(cache_pos.as_int());
}

int32_t Context::AllocateKVCache(int32_t min_reserve)
{
	// Allocate and shift context window
	int n_ctx_used = llama::ctx_used_cells(_pCtx);
	int32_t ctx_size = llama::ctx_size(_pCtx);

	int32_t ctx_chat_max = ctx_size - GetChatBeginOffset().as_int(); // Exclude system prompt
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
	for (auto it = itFirst; it != itLast; ++it)
		it->Discard();

	if (auto removed = RemoveDiscardedBlocks())
	{
		assert(removed == total);
		return total;
	}
	return 0;
}

void Context::DiscardByTTL(int32_t current_turn)
{
	int32_t offset = 0;
	for (auto& block : _blocks)
	{
		if (!block.is_temporary())
			continue;

		if (block.turn + block.ttl < current_turn)
			block.Discard();
	}
}

ContextCursor Context::GetBlockAppendOffset() const
{
	return ContextCursor { GetCache().length() };
}

ContextCursor Context::GetUncachedOffset() const
{
	int32_t pos = 0;
	for (auto& block : _blocks)
	{
		if (block.is_cached())
			pos = std::max(pos, block.cache_position + block.length());
	}
	return ContextCursor { pos };
}

ContextCursor Context::GetChatBeginOffset() const
{
	auto itFind = std::find_if(_blocks.crbegin(), _blocks.crend(), [](const ContextBlock& block) { return not block.flags.IsSet(ContextBlockFlag::Static); });
	if (itFind != _blocks.crend())
		return ContextCursor { itFind->cache_position };
	return ContextCursor { GetCache().length() };
}

std::vector<ContextBlock>::const_iterator Context::GetLastCachedBlock() const
{
	auto itFind = std::find_if(_blocks.crbegin(), _blocks.crend(), [](const ContextBlock& block) { return block.flags.IsSet(ContextBlockFlag::Cached); });
	if (itFind != _blocks.crend())
		return flip_iterator<ContextBlock>(_blocks, itFind);
	return _blocks.cend();
}

void Context::EraseChat() 
{
	if (_blocks.empty())
	{
		cursor_pos = 0;
		chat_begin_pos = 0;
		token_pos = 0;
		return;
	}

	auto itFirst = std::find_if(_blocks.begin(), _blocks.end(), [](const ContextBlock& block) { 
		return !block.flags.IsSet(ContextBlockFlag::Static)
			and block.flags.IsSet(ContextBlockFlag::Cached);
	});
	if (itFirst == _blocks.end())
	{
		ContextCursor block_pos { _cache->length() };
		cursor_pos = block_pos;
		chat_begin_pos = _blocks.back().attn_position + _blocks.back().length();
		token_pos = chat_begin_pos;
	}
	else
	{
		for (auto it = itFirst; it != _blocks.end(); ++it)
			it->Discard();
		RemoveDiscardedBlocks();
		cursor_pos = GetUncachedOffset();
		chat_begin_pos = GetChatBeginOffset();
		token_pos = chat_begin_pos;
		int k = 0;
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

void Context::InsertBlock(ContextBlock&& block, size_t index)
{
	if (index < _blocks.size())
	{
		auto it = _blocks.begin();
		std::advance(it, index);
		_blocks.insert(it, std::move(block));
	}
	else
		AppendBlock(std::move(block));
}

void Context::DumpContext()
{
#if _DEBUG
	for (int32_t i = 0; i < _num_sequences; ++i)
	{
		fig::llm_util::dump_batch_text(*this, i, std::format("prompt_text_{}.txt", i));
		fig::llm_util::dump_batch_tokens(*this, i, std::format("prompt_full_{}.txt", i));
		fig::llm_util::dump_kv_cache(*this, i, std::format("kvcache_{}.txt", i));
	}
	fig::llm_util::dump_kv_cache_cells(*this, "kvcache_alloc.txt");
#endif
}

void Context::RebuildBatch()
{
	auto& cache = GetCache();
	cache.Clear();
	
	for (auto& block : _blocks)
	{
		if (block.is_cached())
			cache.BatchWrite(block.tokens, block.sequenceSlots, block.cache_position, block.attn_position);
	}
}

int32_t Context::Prepend(SequenceSlots seq_id, fig::string text)
{
	auto& cache = GetCache();

	auto prepend_tokens = llama::tokenize(_pVocab, text, false);
	
	// Append to batch
	auto [_, len] = cache.BatchWrite(prepend_tokens, seq_id, -1, prepend_pos.as_int());
	prepend_pos.increment(len);

	DumpContext();
	return toI(prepend_tokens.size());
}

Batch Context::GetCursorView() const
{
	auto& cache = GetCache();
	return cache.GetView(cursor_pos.as_int(), cache.length() - cursor_pos.as_int());
}
