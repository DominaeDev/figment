#pragma once

#include "llm/LLMTypes.h"

namespace fig::llm
{
	class ContextCache
	{
	public:
		ContextCache() = default;
		ContextCache(const ContextCache& other) = delete;
		ContextCache(ContextCache&& other) = delete;
		ContextCache(int32_t size, int32_t n_seq_max);
		~ContextCache();

		Batch GetView(int32_t pos, int32_t length) const;
		void Clear();

		int32_t BatchAllocate(int32_t pos, int32_t length);
		
		// Returns the position and length of tokens written
		std::pair<int32_t, int32_t> BatchWrite(std::span<const Token> tokens, SequenceSlots seq_id, int32_t cache_pos, int32_t attn_position);
		std::pair<int32_t, int32_t> BatchWrite(std::span<const Token> tokens, SequenceSlots seq_id, int32_t cache_pos, ContextCursor& cursor);
		int32_t BatchAddSingle(Token token, Sequences seq_ids, int32_t pos, bool logits = true);
		
		void ShiftBlock(ContextBlock& block, int32_t offset);
		void MoveBlock(ContextBlock& block, int32_t offset);
		void AdjustLength(int32_t offset);

		void InitLogits();
		void BatchSetSequences(int32_t pos, const std::vector<int32_t>& seqIds);
		void BatchSetSequences(int32_t begin, int32_t end, SequenceSlots seq_id);

		int32_t RemoveBlock(const ContextBlock& block);
		void ClearTokensFromIndex(int32_t attn_position);

		int32_t length() const { return _length; }
		int32_t max_size() const { return _max_size; }
		int32_t n_seq_max() const { return _n_seq_max; }

		std::pair<BatchRef, int32_t> GetBatch() { return std::make_pair<BatchRef, int32_t>(std::ref(*_batch.get()), static_cast<int32_t>(_length)); }
		std::pair<BatchCRef, int32_t> GetBatch() const { return std::make_pair<BatchCRef, int32_t>(std::cref(*_batch.get()), static_cast<int32_t>(_length)); }

	private:
		void CopyTokens(int32_t begin, int32_t end, int32_t offset);
		void ShiftTokens(int32_t begin, int32_t end, int32_t offset);
		void ClearToken(int32_t pos);
		void ClearTokens(int32_t begin, int32_t end);

	private:
		std::unique_ptr<Batch> _batch; // Representation of the kv-cache (mirror)
		int32_t _length = 0;
		int32_t _max_size { 0 };
		int32_t _n_seq_max { 1 };
	};
}