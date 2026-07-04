#pragma once

#include "llm/LLMTypes.h"
#include "llm/ContextBlock.h"
#include "llm/ContextCache.h"
#include <optional>
#include <map>

namespace fig::chat
{
	class ChatSession;
}

namespace fig::llm
{
	class ModelState;

	class LLMContext
	{
	public:
		LLMContext(const ModelState& modelState);

		LLMContext() = default;
		LLMContext(LLMContext&& other) = default;
		LLMContext& operator=(LLMContext&& other) = default;
		LLMContext(const LLMContext& other) = default;
		LLMContext& operator=(const LLMContext& other) = default;

		// ContextState
		void Initialize();
		int32_t ReserveTokens(int32_t n_tokens, bool bForce = false);

		// Model info
		const ModelState& GetModel() const noexcept { return *_pModel; }
		ContextPtr GetCtxPtr() const noexcept { return _pCtx; }
		VocabPtr GetVocabPtr() const noexcept { return _pVocab; }
		int32_t GetNumSequences() const noexcept { return _num_sequences; }
		int32_t GetUsedKVCacheCells() const;

		void DiscardByTTL(int32_t current_turn);
		void EraseChat();

		void TokenizeUncached(std::shared_ptr<fig::chat::ChatSession> pSession);
		bool DiscardBlock(const ContextBlock& block);
		
		void RebuildBatch();

		ContextCursor GetBlockAppendOffset() const;
		ContextCursor GetUncachedOffset() const;
		ContextCursor GetChatBeginOffset() const;
		std::vector<ContextBlock>::const_iterator GetLastCachedBlock() const;

		ContextCursor DecodeUncached();

		std::optional<int32_t> RealizeUncachedBlocks();
		std::optional<int32_t> RemoveDiscardedBlocks();

		// Blocks
		int32_t AllocateKVCache(int32_t alloc_min);
		bool RebuildKVCache();

		void AppendBlock(const ContextBlock& block);
		void AppendBlock(ContextBlock&& block);
		void InsertBlock(ContextBlock&& block, size_t index);
		void ClearTokensBelow(int32_t pos);

		// Generation
		Batch GetCursorView() const;
		int32_t Prepend(SequenceSlots seq_id, fig::string text);

		std::vector<ContextBlock>& GetBlocks() noexcept { return _blocks; }
		const std::vector<ContextBlock>& GetBlocks() const noexcept { return _blocks; }
		ContextCache& GetCache() { return *_cache.get(); }
		const ContextCache& GetCache() const { return *_cache.get(); }

		std::map<fig::chat::Role, std::vector<int32_t>> personas;
		fig::chat::Role active_persona = fig::chat::Role::Undefined;
		int32_t last_sequence_index = 0;

		// Positions
		ContextCursor cursor_pos {};		// (kv-cache) current position (read)
		ContextCursor response_pos {};		// (token) start of response, including prompt template preamble
		ContextCursor prepend_pos {};		// (token) start of response, excluding prompt template preamble
		ContextCursor chat_begin_pos {};	// (kv-cache) chat position
		ContextCursor token_pos {};			// (token) Current token/attention position (write)
	
	private:
		void DumpContext();
		std::optional<int32_t> DecodeSingleUncached(ContextBlock& block);

	private:
		const ModelState* _pModel = nullptr;
		ContextPtr _pCtx = nullptr;
		VocabPtr _pVocab = nullptr;
		int32_t _num_sequences {};
		float _fKeepRatio {};
		std::vector<ContextBlock> _blocks {};
		std::shared_ptr<ContextCache> _cache {};
	};
}