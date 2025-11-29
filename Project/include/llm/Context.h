#include "llm/LLMTypes.h"
#include "llm/ContextBlock.h"
#include "llm/ContextCache.h"

#pragma once

class ContextSequence
{
public:
	ContextSequence() = default;
	ContextSequence(llama_context* pCtx, int32_t n_seq_max);

	void RefreshBlockPositions();
	int32_t RemoveBlock(const ContextBlock& block, bool bShift = true);
	int32_t RemoveBlocks(std::vector<ContextBlock>::const_iterator begin, std::vector<ContextBlock>::const_iterator end, bool bShift = true);
	int32_t EraseChat();
	void EraseVolatile();

	bool RebuildKVCache();
	int32_t AllocateKVCache(int32_t alloc_min);

	int32_t GetBlockAppendOffset() const;

	std::pair<int32_t, bool> DecodeTokens(const std::vector<llama_token>& tokens, int32_t pos, SequenceId seq_id); // std::expected?
	int32_t DecodeUncached(int32_t cursor_pos);
	int32_t DecrementTTL(int32_t time);
	
	void AppendBlock(const ContextBlock& block);
	void AppendBlock(ContextBlock&& block);
	void ClearTokensBelow(int32_t pos);


	int32_t get_max_position() const;

	std::vector<ContextBlock>& GetBlocks() noexcept { return _blocks; }
	const std::vector<ContextBlock>& GetBlocks() const noexcept { return _blocks; }
	ContextCache& GetCache() { return *_cache.get(); }
	const ContextCache& GetCache() const { return *_cache.get(); }

	int32_t GetNumSequences() const { return n_seq_max; }

public:
	llama_context* pCtx = nullptr;

	int32_t response_pos = 0;					// start of response, including prompt template preamble
	int32_t prepend_pos = 0;					// start of response, excluding prompt template preamble
	int32_t cursor_pos = 0;						// current position (read)
	int32_t chat_begin_pos = 0;					// chat position
	int32_t n_seq_max = 1;

private:
	std::shared_ptr<ContextCache> _cache;
	std::vector<ContextBlock> _blocks {};
};

struct ContextState
{
	ContextState() = default;
	ContextState(ContextState&& other) = default;
	ContextState(const ContextState& other) = default;
	ContextState(const ModelState& model, int32_t n_seq = 1);
	ContextState& operator=(const ContextState& other) = default;

	void Initialize();
	bool ReserveTokens(int32_t n_tokens, bool bForce = false);

	int32_t get_max_position() const;
	[[nodiscard]] SequenceIndices get_all_sequences() const noexcept;

	llama_context* pCtx = nullptr;
	const llama_vocab* pVocab = nullptr;

	std::map<Role, std::vector<int32_t>> personas;
	Role activePersona = Role::Undefined;

	ContextSequence sequence;
	int32_t previous_sequence_index = 0;
	int32_t num_sequences = 1;
	int32_t max_tokens = 0;

};