#include <pch.h>
#include "llm/Context.h"
#include "llm/LlamaApi.h"
#include "llm/LLMUtility.h"
#include "llm/LLMTemplate.h"
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
		return AllocateKVCache(ctx_reserve);
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
			content = llm_tmpl::apply_chat_template_prefix(block.role, content, block.name); //! name?
		else if (block.role == Role::System)
			content = llm_tmpl::apply_chat_template({ Message { block.role, content, block.name } }, false);
		else
		{
			fig::llm_util::complete_message(content);
			content = llm_tmpl::apply_chat_template({ Message { block.role, content, block.name } }, false);
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
		return ContextCursor(0);
	}

	int32_t last_position = -1;
	int32_t attn_position = 0;

	for (size_t i = 0; i < _blocks.size(); ++i)
	{
		auto& block = _blocks[i];
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
		auto [batch_pos, _] = GetCache().BatchWrite(block.tokens, block.sequenceSlots, block.attn_position);
		block.cache_position = batch_pos;

		if (block.is_continuation())
			GetCache().InitLogits(); // Enable logits for last token

		// ... and decode it.
		Batch batch_view = GetCache().GetView(block.cache_position, block.length());
//		llm_util::dump_batch_tokens(batch_view, batch_view.n_tokens, 0, _pVocab, "add.txt");
		if (llama::ctx_decode(_pCtx, batch_view) != llama::DecodeError::NoError)
		{
			llama::free(batch_view);
			return ContextCursor::Invalid; // Error
		}

		block.flags.Set(ContextBlockFlag::Cached);
		block.flags.Unset(ContextBlockFlag::Contination);
		attn_position = std::max(attn_position, block.attn_position + block.length());
	}

	cursor_pos = _blocks.back().cache_position + _blocks.back().length();
	return cursor_pos;
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
	return r == 0;
}

std::optional<int32_t> Context::RemoveDiscardedBlocks()
{
	int32_t tokens_removed = 0;
#if _DEBUG
	int32_t n_used_before = llama::ctx_used_cells(_pCtx);
#endif
	
	auto& cache = GetCache();

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

//		DumpContext();

		// Remove from cache
		int32_t block_pos = block.cache_position;
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
			// Shift up subsequent blocks
			for (size_t shift_idx = i; shift_idx < _blocks.size(); ++shift_idx)
			{
				auto& shift_block = _blocks[shift_idx];
				if (shift_block.cache_position > block_pos)
				{
					assert(shift_block.attn_position >= 0);
					if (shift_block.is_cached())
					{
						cache.MoveBlock(shift_block, shift);
						llama::ctx_move(_pCtx, shift_block, shift);
						shift_block.attn_position += shift;
//						DumpContext();
					}
				}
			}
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
	
	cache.ClearTokensFrom(cache.length() + total_shift);
	cursor_pos.increment(total_shift);

	DumpContext();
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
	GetCache().ClearTokensFrom(pos);
}

int32_t Context::AllocateKVCache(int32_t min_reserve)
{
	// Allocate and shift context window
	int n_ctx_used = llama::ctx_used_cells(_pCtx);
	int32_t ctx_size = llama::ctx_size(_pCtx);

	int32_t ctx_chat_max = ctx_size - chat_begin_pos.as_int(); // Exclude system prompt
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
	if (auto pCache = _cache.get())
		return ContextCursor(pCache->length());
	return ContextCursor::Invalid;
}

std::vector<ContextBlock>::const_iterator Context::GetLastCachedBlock() const
{
	auto itFind = std::find_if(_blocks.crbegin(), _blocks.crend(), [](const ContextBlock& block) { return block.flags.IsSet(ContextBlockFlag::Cached); });
	if (itFind != _blocks.crend())
		return flip_iterator<ContextBlock>(_blocks, itFind);
	return _blocks.cend();
}

ContextCursor Context::EraseChat() 
{
	auto itFirst = std::find_if(_blocks.begin(), _blocks.end(), [](const ContextBlock& block) { return !block.flags.IsSet(ContextBlockFlag::Static); });
	if (itFirst == _blocks.end())
	{
		ContextCursor block_pos { _cache->length() };
		cursor_pos = block_pos;
		chat_begin_pos = block_pos;
		return block_pos;
	}
	else
	{
		chat_begin_pos = itFirst->cache_position;
		for (auto it = itFirst; it != _blocks.end(); ++it)
			it->Discard();
		return chat_begin_pos;
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
	
	int32_t offset = 0;
	for (auto& block : _blocks)
	{
		if (block.is_cached())
		{
			cache.BatchWrite(block.tokens, block.sequenceSlots, offset);
			offset += block.length();
		}
	}
}

int32_t Context::Prepend(SequenceSlots seq_id, fig::string text)
{
	auto& cache = GetCache();

	auto prepend_tokens = llama::tokenize(_pVocab, text, false);
	
	// Append to batch
	cache.BatchWrite(prepend_tokens, seq_id, prepend_pos.as_int());
	prepend_pos.increment(prepend_tokens.size());

	DumpContext();
	return toI(prepend_tokens.size());
}

Batch Context::GetCursorView() const
{
	auto& cache = GetCache();
	return cache.GetView(cursor_pos.as_int(), cache.length() - cursor_pos.as_int());
}
