#ifndef LLAMA_API_H__
#define LLAMA_API_H__
#pragma once

#include "llm/LLMTypes.h"

namespace fig::llm::llama
{
	std::vector<Token> tokenize(VocabPtr pModel, fig::string prompt, bool add_special = false);
	fig::string untokenize(VocabPtr pVocab, Token token);

	Batch init_batch(int32_t ctx_size, int32_t n_seq_max);
	bool init_embedding_batch(ModelPtr pModel, ContextPtr pCtx, const std::vector<Token>& tokens, Batch& out_pBatch);
	Batch create_batch(std::span<Token> tokens, std::span<LlamaSequence> seqs, int32_t n_seq_max, int32_t position, bool logits = false);
	Batch create_batch_view(const Batch& batch, int32_t position, int32_t length);

	enum class DecodeError
	{
		NoError,
		Failed,
		NoContiguousBlock,
	};
	inline DecodeError ctx_decode(ContextPtr pCtx, const Batch& batch_view)
	{
		int32_t r = llama_decode(pCtx, batch_view);
		if (r == 0)
			return DecodeError::NoError;
		if (r > 0)
			return DecodeError::NoContiguousBlock;
		return DecodeError::Failed;
	}

	inline bool ctx_remove(ContextPtr pCtx, int32_t begin, int32_t end = -1)
	{
		return llama_kv_self_seq_rm(pCtx, -1, begin, end);
	}

	inline bool ctx_remove(ContextPtr pCtx, LlamaSequence seq_id, int32_t begin, int32_t end)
	{
		llama_kv_self_seq_rm(pCtx, seq_id, begin, end);
	}

	bool ctx_remove(ContextPtr pCtx, SequenceId seq_ids, int32_t begin, int32_t end = -1);

	inline void ctx_move(ContextPtr pCtx, LlamaSequence seq_id, int32_t begin, int32_t end, int32_t offset)
	{
		llama_kv_self_seq_add(pCtx, seq_id, begin, end, offset);
	}

	inline void ctx_copy_sequence(ContextPtr pCtx, LlamaSequence seq_from, LlamaSequence seq_to, int32_t begin, int32_t end)
	{
		llama_kv_self_seq_cp(pCtx, seq_from, seq_to, begin, end);
	}

	void ctx_copy_sequence(ContextPtr pCtx, SequenceId seq_from, SequenceId seq_to, int32_t begin, int32_t end);

	inline void ctx_clear(ContextPtr pCtx)
	{
		llama_kv_self_clear(pCtx);
	}

	inline int32_t ctx_size(ContextPtr pCtx)
	{
		return llama_n_ctx(pCtx);
	}

	inline int32_t ctx_used_cells(ContextPtr pCtx)
	{
		return llama_kv_self_used_cells(pCtx);
	}

	inline VocabPtr get_vocab(ModelPtr pModel)
	{
		return llama_model_get_vocab(pModel);
	}

	inline void ctx_defrag(ContextPtr pCtx)
	{
		llama_kv_self_defrag(pCtx);
	}

	inline void ctx_update(ContextPtr pCtx)
	{
		llama_kv_self_update(pCtx);
	}

	inline void free(ModelPtr pModel)
	{
		llama_model_free(pModel);
	}

	inline void free(ContextPtr pCtx)
	{
		llama_free(pCtx);
	}

	inline void free(SamplerPtr pSampler)
	{
		llama_sampler_free(pSampler);
	}

	inline void free(Batch& batch)
	{
		llama_batch_free(batch);
		batch.pos = nullptr;
		batch.token = nullptr;
		batch.embd = nullptr;
		batch.logits = nullptr;
		batch.n_seq_id = nullptr;
		batch.seq_id = nullptr;
		batch.n_tokens = 0;
	}
}

#endif