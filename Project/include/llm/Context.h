#include "llm/LLMTypes.h"
#include "llm/ContextBlock.h"
#include "llm/ContextCache.h"

#pragma once

class Context
{
public:
	Context(const ModelState& model);
	
	Context() = default;
	Context(Context&& other) = default;
	Context& operator=(Context&& other) = default;
	Context(const Context& other) = default;
	Context& operator=(const Context& other) = default;

	// ContextState
	void Initialize();
	bool ReserveTokens(int32_t n_tokens, bool bForce = false);

	// Model info
	const ModelState& GetModel() const noexcept { return *_pModel; }
	ContextPtr GetCtxPtr() const noexcept { return _pCtx; }
	VocabPtr GetVocabPtr() const noexcept { return _pVocab; }
	int32_t GetNumSequences() const noexcept { return _num_sequences; }
	int32_t GetUsedKVCacheCells() const;

	std::map<Role, std::vector<int32_t>> personas;
	Role activePersona = Role::Undefined;
	int32_t previous_sequence_index = 0;

	// ContextSequence

	void RefreshBlockPositions();
	int32_t RemoveBlock(const ContextBlock& block, bool bShift = true);
	int32_t RemoveBlocks(std::vector<ContextBlock>::const_iterator begin, std::vector<ContextBlock>::const_iterator end, bool bShift = true);
	int32_t EraseChat();
	void EraseVolatile();
	int32_t GetBlockAppendOffset() const;
	int32_t DecrementTTL(int32_t time);

	// Blocks
	int32_t AllocateKVCache(int32_t alloc_min);
	bool RebuildKVCache();
	std::pair<int32_t, bool> DecodeTokens(const std::vector<llama_token>& tokens, int32_t pos, SequenceId seq_id); // std::expected?
	int32_t DecodeUncached(int32_t cursor_pos);
	
	void AppendBlock(const ContextBlock& block);
	void AppendBlock(ContextBlock&& block);
	void ClearTokensBelow(int32_t pos);

	std::vector<ContextBlock>& GetBlocks() noexcept { return _blocks; }
	const std::vector<ContextBlock>& GetBlocks() const noexcept { return _blocks; }
	ContextCache& GetCache() { return *_cache.get(); }
	const ContextCache& GetCache() const { return *_cache.get(); }

	// Positions
	int32_t response_pos = 0;					// start of response, including prompt template preamble
	int32_t prepend_pos = 0;					// start of response, excluding prompt template preamble
	int32_t cursor_pos = 0;						// current position (read)
	int32_t chat_begin_pos = 0;					// chat position

private:
	const ModelState* _pModel = nullptr;
	ContextPtr _pCtx = nullptr;
	VocabPtr _pVocab = nullptr;
	int32_t _num_sequences {};
	std::vector<ContextBlock> _blocks {};
	std::shared_ptr<ContextCache> _cache {};
};