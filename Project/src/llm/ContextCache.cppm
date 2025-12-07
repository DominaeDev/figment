export module Context:ContextCache;

import Types;
import LLMTypes;

export 
{
	class ContextCache
	{
	public:
		ContextCache() = default;
		ContextCache(const ContextCache& other) = delete;
		ContextCache(ContextCache&& other) = delete;
		ContextCache(int32_t size, int32_t n_seq_max);
		~ContextCache();

		Batch GetBatchView(int32_t pos, int32_t length) const;
		void Clear();

		int32_t BatchAllocate(int32_t pos, int32_t length);
		int32_t BatchWrite(std::span<const Token> tokens, SequenceId seq_id, int32_t pos);
		int32_t BatchAddSingle(Token token, SequenceIndices seq_ids, int32_t pos);

		void InitLogits();
		void BatchSetSequences(int32_t pos, const std::vector<int32_t>& seqIds);
		void BatchSetSequences(int32_t from, int32_t length, SequenceId seq_id);

		int32_t BatchRemove(int32_t begin, int32_t end);
		int32_t ClearRange(int32_t begin, int32_t end);
		void ClearTokensFrom(int32_t from);

		int32_t length() const { return _length; }
		int32_t max_size() const { return _max_size; }
		int32_t n_seq_max() const { return _n_seq_max; }

		std::pair<std::reference_wrapper<Batch>, int32_t> GetBatch() { return std::make_pair<std::reference_wrapper<Batch>, int32_t>(static_cast<Batch&>(*_batch.get()), static_cast<int32_t>(_length)); }
		std::pair<std::reference_wrapper<Batch>, int32_t> GetBatch() const { return std::make_pair<std::reference_wrapper<Batch>, int32_t>(static_cast<Batch&>(*_batch.get()), static_cast<int32_t>(_length)); }

	private:
		void CopyTokens(int32_t from, int32_t to);
		int32_t ShiftTokens(int32_t pos, int32_t len, int32_t offset);

	private:
		std::unique_ptr<Batch> _batch; // Representation of the kv-cache (mirror)
		int32_t _length = 0;
		int32_t _max_size { 0 };
		int32_t _n_seq_max { 1 };
	};
}
