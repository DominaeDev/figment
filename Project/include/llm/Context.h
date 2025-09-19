#include "llm/LLMTypes.h"

#pragma once

enum class ContextBlockFlag : int32_t
{
	None		= 0,
	Static		= 1 << 0,	// Static instruction
	Cached		= 1 << 1,	// Currently cached
	Volatile	= 1 << 2,	// Discarded immediately after use
};
DEFINE_ENUM_FLAGS(ContextBlockFlag, int32_t);

struct ContextBlock 
{
	string responseId;
	Role role;
	string name;
    string content;
	std::vector<int32_t> tokens;
	ContextBlockFlag flags = ContextBlockFlag::None;
	int32_t offset {};
	int ttl = -1;

	inline bool is_cached() const { return CheckEnumFlag(flags, ContextBlockFlag::Cached); }
	inline bool is_volatile() const { return CheckEnumFlag(flags, ContextBlockFlag::Volatile); }
	inline bool is_temporary() const { return ttl > 0; }

	inline int32_t length() const { return (int32_t)(tokens.size()); }
};

enum class SequenceId : int32_t
{
	None	= 0,
	Bot1	= 1 << 0,
	Bot2	= 1 << 1,
	Bot3	= 1 << 2,
	Bot4	= 1 << 3,
	Shared = Bot1 | Bot2 | Bot3 | Bot4,
};
DEFINE_ENUM_FLAGS(SequenceId, int32_t);

using SequenceList = std::vector<SequenceId>;

struct ContextSequence
{
	llama_context* pCtx = nullptr;
	int32_t seq_index = 0;					// Sequence index
	llama_batch batch {};					// Representation of the kv-cache (mirror)
	std::vector<ContextBlock> blocks {};
	int32_t persona_pos = 0;				// persona insertion point
	int32_t response_pos = 0;				// start of response, including prompt template preamble
	int32_t prepend_pos = 0;				// start of response, not including prompt template preamble
	int32_t blocks_pos = 0;					// position of first message block
	int32_t current_pos = 0;				// current position (cursor)
	
	int32_t AssignBlockPositions();
	int32_t RemoveAndShift(const llama_vocab* pVocab, std::vector<ContextBlock>::iterator itBegin, std::vector<ContextBlock>::iterator itEnd);
	bool RebuildKVCache();
};

struct ContextState
{
	llama_context* pCtx = nullptr;			// Non-owned
	const llama_vocab* pVocab = nullptr;	// Non-owned
	std::vector<int32_t> system_tokens;
	std::map<Role, std::vector<int32_t>> personas;
	Role activePersona = Role::Undefined;

	size_t active_sequence = 0;
	std::vector<ContextSequence> sequences;
	constexpr ContextSequence& current_sequence() {
		return sequences[active_sequence];
	}
	constexpr const ContextSequence& current_sequence() const {
		return sequences[active_sequence];
	}
};