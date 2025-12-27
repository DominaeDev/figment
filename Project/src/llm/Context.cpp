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

int32_t Context::TokenizeUncached(ChatSession& session)
{
	// Tokenize uncached messages
	int32_t num_tokens = 0;
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

		block.offset = -1;
		block.tokens = llama::tokenize(_pVocab, content, false);
		num_tokens += block.length();
	}
	return num_tokens;
}

int32_t Context::DecodeUncached()
{
	int32_t last_offset = -1;
	int32_t offset = 0;
	int32_t persona_offset = -1;
	for (size_t i = 0; i < _blocks.size(); ++i)
	{
		auto& block = _blocks[i];
		if (block.is_cached())
		{
			offset = std::max(block.offset + block.length(), offset);
			if (block.is_persona() && persona_offset < 0)
				persona_offset = offset;
			continue;
		}

		assert(last_offset <= offset);
		assert(block.length() > 0);

		block.offset = offset;
		last_offset = offset;

		if (block.is_persona()) // Personas overlap
		{
			if (persona_offset < 0)
				persona_offset = offset;
			else
				block.offset = persona_offset;
		}

		GetCache().BatchWrite(block.tokens, block.sequenceId, block.offset); //! Personas

		// Decode
		std::vector<llama_seq_id> seqs = block.get_sequence_ids(_num_sequences);
		assert(!block.is_persona() || seqs.size() == 1);

		llama_batch batch_view = llama::create_batch(block.tokens, seqs, _num_sequences, block.offset, block.is_continuation());
		if (llama_decode(_pCtx, batch_view) != 0)
		{
			llama_batch_free(batch_view);
			return -1; // Error
		}

		block.flags.Set(ContextBlockFlag::Cached);
		block.flags.Unset(ContextBlockFlag::Contination);
		offset = std::max(offset, block.offset + block.length());
		llama_batch_free(batch_view);
	}
	cursor_pos = std::max(cursor_pos, offset);
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
		int32_t blocks_pos = chat_begin_pos;

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
	
	// Remove
	for (int32_t i = toI(_blocks.size()) - 1; i >= 0; --i)
	{
		size_t idx = toUZ(i);
		auto& block = _blocks[idx];
		if (not block.is_discarded())
			continue;

		if (not block.is_cached())
		{
			_blocks.erase(_blocks.cbegin() + idx);
			continue;
		}

		if (not llama::ctx_remove(_pCtx, block.offset, block.offset + block.length()))
			return std::nullopt; // Error

		int32_t removed = block.length();
		tokens_removed += removed;

		_cache->ClearRange(block.offset, block.offset + removed);
		container_remove_at(_blocks, idx);

		// Shift up subsequent blocks
		int32_t shift = -removed;
		for (size_t shift_idx = idx; shift_idx < _blocks.size(); ++shift_idx)
		{
			auto& shift_block = _blocks[shift_idx];
			if (shift_block.is_cached())
			{
				LlamaSequence seq_id = shift_block.get_any_sequence_id();
				llama::ctx_move(_pCtx, seq_id, shift_block.offset, shift_block.offset + shift_block.length(), shift);

				_cache->ShiftTokens(shift_block.offset, toI(shift_block.length()), shift);
				shift_block.offset += shift;
			}
		}
	}

	if (tokens_removed == 0)
		return 0;

	llama::ctx_defrag(_pCtx);
	llama::ctx_update(_pCtx);

#if _DEBUG
	int32_t n_used_after = llama::ctx_used_cells(_pCtx);
	assert(n_used_after == n_used_before - tokens_removed);
#endif
	
	// Realign blocks (shift)
	if constexpr (Disabled)
	{
		int32_t curr_offset = 0;
		for (size_t idx = 0; idx < _blocks.size(); ++idx)
		{
			auto& block = _blocks[idx];
			if (!block.is_static() && block.offset > curr_offset)
			{
				int32_t shift = curr_offset - block.offset;
				for (auto it = _blocks.begin() + idx; it < _blocks.end(); ++it)
				{
					auto& shift_block = *it;
					if (shift_block.is_cached())
					{
						LlamaSequence seq_id = shift_block.get_any_sequence_id();
						llama::ctx_move(_pCtx, seq_id, shift_block.offset, shift_block.offset + shift_block.length(), shift);

						_cache->ShiftTokens(block.offset, toI(block.length()), shift);
						shift_block.offset += shift;
					}
				}
			}
			curr_offset = std::max(curr_offset, block.offset + block.length());
		}
	}

	cursor_pos -= tokens_removed;

	DumpContext();

	return tokens_removed;
}

/*int32_t Context::RemoveBlocks(std::vector<ContextBlock>::const_iterator begin, std::vector<ContextBlock>::const_iterator end, bool bShift)
{
	int32_t idx_from = toI(std::distance(_blocks.cbegin(), begin));
	int32_t idx_to = toI(std::distance(_blocks.cbegin(), end)) - 1;

#if _DEBUG
	int32_t n_used = llama::get_used_cells(_pCtx);
#endif
	int32_t total_length = 0;

	int32_t pos_remove_begin = _blocks[idx_from].offset;

	// Remove tokens
	for (int32_t i = idx_to; i >= idx_from; --i)
	{
		auto& block = _blocks[i];
		assert(block.sequenceId != SequenceId::None);

		if (block.is_cached())
		{
			if (!llama_kv_self_seq_rm(_pCtx, -1, block.offset, block.offset + block.length()))
				return 0;
			total_length += block.length();
		}
		_blocks.erase(_blocks.begin() + i);
	}

	if (bShift)
	{
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
		_cache->BatchRemove(pos_remove_begin, pos_remove_end);
	}

	llama_kv_self_defrag(_pCtx);

#if _DEBUG
	try
	{
		llama_kv_self_update(_pCtx);
		int32_t n_used_after = llama::get_used_cells(_pCtx);
		assert(n_used_after < n_used);
	}
	catch (std::exception& e)
	{
		LogLn(std::format("Exception: {}", e.what()));
	}
#endif

	return (int32_t)-total_length;
}*/

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

int32_t Context::GetBlockAppendOffset() const
{
	int32_t offset = 0;
	for (auto& block : _blocks)
		offset = std::max(offset, block.offset + block.length());
	return offset;
}

std::vector<ContextBlock>::const_iterator Context::GetLastCachedBlock() const
{
	auto itFind = std::find_if(_blocks.crbegin(), _blocks.crend(), [](const ContextBlock& block) { return block.flags.IsSet(ContextBlockFlag::Cached); });
	if (itFind != _blocks.crend())
		return flip_iterator<ContextBlock>(_blocks, itFind);
	return _blocks.cend();
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
		for (auto it = itFirst; it != _blocks.end(); ++it)
			it->Discard();
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
			cache.BatchWrite(block.tokens, block.sequenceId, offset);
			offset += block.length();
		}
	}
}

int32_t Context::Prepend(SequenceId seq_id, fig::string text)
{
	auto& cache = GetCache();

	auto prepend_tokens = llama::tokenize(_pVocab, text, false);
	// Append to batch
	cache.BatchWrite(prepend_tokens, seq_id, prepend_pos);
	prepend_pos += toI(prepend_tokens.size());

	DumpContext();
	return toI(prepend_tokens.size());
}

Batch Context::GetCursorView() const
{
	auto& cache = GetCache();
	return cache.GetView(cursor_pos, cache.length() - cursor_pos);
}