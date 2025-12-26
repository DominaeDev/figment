#include "llm/LLMTypes.h"
#include "llm/ContextBlock.h"
#include "llm/ContextCache.h"
#include <optional>
#include <map>

#pragma once

class ChatSession;

namespace fig::llm
{
	class ModelState;

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

		void RefreshBlockPositions();
		void DiscardByTTL(int32_t current_turn);
		int32_t EraseChat();

		int32_t TokenizeUncached(ChatSession& session);
		bool DiscardBlock(const ContextBlock& block);
		
		void RebuildBatch();

		int32_t GetBlockAppendOffset() const;
		std::vector<ContextBlock>::const_iterator GetLastCachedBlock() const;

		int32_t DecodeUncached();
		std::optional<int32_t> RemoveDiscardedBlocks();

		// Blocks
		int32_t AllocateKVCache(int32_t alloc_min);
		bool RebuildKVCache();

		void AppendBlock(const ContextBlock& block);
		void AppendBlock(ContextBlock&& block);
		void ClearTokensBelow(int32_t pos);

		std::vector<ContextBlock>& GetBlocks() noexcept { return _blocks; }
		const std::vector<ContextBlock>& GetBlocks() const noexcept { return _blocks; }
		ContextCache& GetCache() { return *_cache.get(); }
		const ContextCache& GetCache() const { return *_cache.get(); }

		std::map<Role, std::vector<int32_t>> personas;
		Role active_persona = Role::Undefined;
		int32_t last_sequence_index = 0;

		// Positions
		int32_t response_pos = 0;					// start of response, including prompt template preamble
		int32_t prepend_pos = 0;					// start of response, excluding prompt template preamble
		int32_t cursor_pos = 0;						// current position (read)
		int32_t chat_begin_pos = 0;					// chat position
	
	private:
		void DumpContext();

	private:
		const ModelState* _pModel = nullptr;
		ContextPtr _pCtx = nullptr;
		VocabPtr _pVocab = nullptr;
		int32_t _num_sequences {};
		std::vector<ContextBlock> _blocks {};
		std::shared_ptr<ContextCache> _cache {};
	};
}