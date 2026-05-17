#include <pch.h>
#include <cassert>
#include "llm/LlamaApi.h"
#include "llm/LLMUtility.h"

using namespace fig::llm;
using namespace fig::util;

namespace fig::llm::llama
{
	std::vector<Token> tokenize(VocabPtr pVocab, fig::string prompt, bool add_special)
	{
		std::vector<Token> prompt_tokens(1024);
		const int32_t n_prompt_tokens = llama_tokenize(pVocab, prompt.c_str(), (int32_t)prompt.size(), prompt_tokens.data(), (int32_t)prompt_tokens.size(), add_special, false);
		if (n_prompt_tokens < 0)
		{
			prompt_tokens.resize(-n_prompt_tokens);
			if (llama_tokenize(pVocab, prompt.c_str(), (int32_t)prompt.size(), prompt_tokens.data(), (int32_t)prompt_tokens.size(), add_special, false) < 0)
			{
				// Error
				return std::vector<Token> {};
			}
		}
		else
		{
			prompt_tokens.resize(n_prompt_tokens);
		}
		return prompt_tokens;
	}

	fig::string untokenize(VocabPtr pVocab, Token token)
	{
		// convert the token to a string, print it and add it to the response
		char buf[256];
		int n = llama_token_to_piece(pVocab, token, buf, sizeof(buf), 0, false);
		if (n < 0)
			return "";

		return fig::string(buf, n);
	}

	Batch init_batch(int32_t ctx_size, int32_t n_seq_max)
	{
		// Prepare a batch for the prompt
		Batch batch = llama_batch_init(ctx_size, NULL, n_seq_max);
		batch.n_tokens = 0;

		for (size_t i = 0; i < ctx_size; ++i)
		{
			batch.pos[i] = (int32_t)i;
			batch.token[i] = 0;
			batch.n_seq_id[i] = 0;
			for (size_t itSeq = 1; itSeq < n_seq_max; ++itSeq)
				batch.seq_id[i][itSeq] = -1;
			batch.logits[i] = false;
		}
		return batch;
	}

	Batch create_batch(std::span<Token> tokens, std::span<Sequence> seqs, int32_t n_seq_max, int32_t position, bool logits)
	{
		// Prepare a batch for the prompt
		Batch batch = llama_batch_init(toI(tokens.size()), NULL, n_seq_max);
		batch.n_tokens = toI(tokens.size());
		batch.embd = nullptr;

		size_t i = 0;
		for (auto token : tokens)
		{
			batch.pos[i] = (int32_t)i + position;
			batch.token[i] = token;
			batch.n_seq_id[i] = toI(seqs.size());
			batch.logits[i] = 0;
			std::copy(seqs.begin(), seqs.end(), batch.seq_id[i]);
			++i;
		}
		if (i > 0)
			batch.logits[i - 1] = logits;
		return batch;
	}

	Batch create_batch_view(const Batch& batch, int32_t position, int32_t length)
	{
		return Batch {
			length,
			batch.token + position,
			nullptr,
			batch.pos + position,
			batch.n_seq_id + position,
			batch.seq_id + position,
			batch.logits + position,
		};
	}

	bool init_embedding_batch(ModelPtr pModel, ContextPtr pCtx, const std::vector<Token>& tokens, Batch& out_pBatch)
	{
		const int32_t ctx_size = llama_n_ctx(pCtx);

		// Prepare a batch for the prompt
		Batch batch = llama_batch_init(ctx_size, NULL, 1);
		int32_t num_tokens = std::min((int32_t)tokens.size(), ctx_size);

		// Add tokens to batch
		for (int i = 0; i < num_tokens; ++i)
		{
			batch.token[i] = tokens[i];
			batch.pos[i] = i;		// Position in sequence
			batch.n_seq_id[i] = 1;	// This token belongs to 1 sequence
			batch.seq_id[i][0] = 0;	// Sequence ID 0 //! @seq
			batch.logits[i] = true;
		}
		batch.n_tokens = num_tokens;

		out_pBatch = batch;
		return true;
	}

	DecodeError ctx_decode(ContextPtr pCtx, const Batch& batch_view)
	{
		if constexpr (Debugging)
		{
			LogLn(std::format("llama_decode(pCtx, t={}, pos={}, len={}, n_seq={}, seq={});", batch_view.token[0], batch_view.pos[0], batch_view.n_tokens, batch_view.n_seq_id[0], batch_view.seq_id[0][0]));
		}
		int32_t r = llama_decode(pCtx, batch_view);
		if (r == 0)
			return DecodeError::NoError;
		if (r > 0)
			return DecodeError::NoContiguousBlock;
		return DecodeError::Failed;
	}

	bool ctx_remove(ContextPtr pCtx, int32_t begin, int32_t end)
	{
		if constexpr (Debugging)
		{
			LogLn(std::format("llama_kv_self_seq_rm(pCtx, -1, {}, {});", begin, end));
			bool r = llama_kv_self_seq_rm(pCtx, -1, begin, end);
			assert(r); // When is this false?
			return r;
		}
		else
		{
			return llama_kv_self_seq_rm(pCtx, -1, begin, end);
		}
	}

	bool ctx_remove(ContextPtr pCtx, Sequence seq_id, int32_t begin, int32_t end)
	{
		if constexpr (Debugging)
		{
			LogLn(std::format("llama_kv_self_seq_rm(pCtx, {}, {}, {});", seq_id, begin, end));
			bool r = llama_kv_self_seq_rm(pCtx, seq_id, begin, end);
			assert(r); // When is this false?
			return r;
		}
		else
		{
			return llama_kv_self_seq_rm(pCtx, seq_id, begin, end);
		}
	}

	bool ctx_remove(ContextPtr pCtx, const ContextBlock& block)
	{
		if (block.sequenceSlots == SequenceSlot::Shared)
			return ctx_remove(pCtx, block.attn_position, block.attn_position + block.length());
		else
			return ctx_remove(pCtx, block.sequenceSlots, block.attn_position, block.attn_position + block.length());
	}

	bool ctx_remove(ContextPtr pCtx, SequenceSlots seq_slots, int32_t begin, int32_t end)
	{
		auto seq_ids = fig::llm::util::get_sequence_indices(seq_slots, toI(AllSequenceSlots.size()));
		for (auto seq_id : seq_ids)
		{
			if (!ctx_remove(pCtx, seq_id, begin, end))
				return false;
		}
		return true;
	}

	void ctx_copy_sequence(ContextPtr pCtx, SequenceSlots seq_from, SequenceSlots seq_to, int32_t begin, int32_t end)
	{
		auto from = fig::llm::util::get_sequence_indices(seq_from, toI(AllSequenceSlots.size()))[0];
		auto to = fig::llm::util::get_sequence_indices(seq_to, toI(AllSequenceSlots.size()))[0];

		if constexpr (Debugging)
		{
			LogLn(std::format("llama_kv_self_seq_cp(pCtx, {}, {}, {}, {});", from, to, begin, end));
		}
		llama_kv_self_seq_cp(pCtx, from, to, begin, end);
	}

	void ctx_move(ContextPtr pCtx, ContextBlock& block, int32_t offset)
	{
		auto seq_id = block.get_any_sequence_id();
		ctx_move(pCtx, seq_id, block.attn_position, block.attn_position + block.length(), offset);
	}

	void ctx_move(ContextPtr pCtx, Sequence seq_id, int32_t begin, int32_t end, int32_t offset)
	{
		if constexpr (Debugging)
		{
			LogLn(std::format("llama_kv_self_seq_add(pCtx, {}, {}, {}, {});", seq_id, begin, end, offset));
		}
		llama_kv_self_seq_add(pCtx, seq_id, begin, end, offset);
	}

	void ctx_copy_sequence(ContextPtr pCtx, Sequence seq_from, Sequence seq_to, int32_t begin, int32_t end)
	{
		if constexpr (Debugging)
		{
			LogLn(std::format("llama_kv_self_seq_cp(pCtx, {}, {}, {}, {});", seq_from, seq_to, begin, end));
		}
		llama_kv_self_seq_cp(pCtx, seq_from, seq_to, begin, end);
	}
}