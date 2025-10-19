#include "llm/LLMTypes.h"

#pragma once

enum class ContextBlockFlag : int32_t
{
	None		= 0,
	Static		= 1 << 0,	// Static instruction
	Cached		= 1 << 1,	// Is currently in kv-cache
	Volatile	= 1 << 2,	// Should be discarded immediately
	Persona		= 1 << 3,	// Persona
};
DEFINE_ENUM_FLAGS(ContextBlockFlag, int32_t);

struct ContextBlock 
{
	string responseId;
	Role role = Role::Undefined;
	string name;
    string content;
	std::vector<int32_t> tokens;
	ContextBlockFlag flags = ContextBlockFlag::None;
	int32_t offset = 0;
	int ttl = 0;

	inline bool is_static() const		{ return CheckEnumFlag(flags, ContextBlockFlag::Static); }
	inline bool is_cached() const		{ return CheckEnumFlag(flags, ContextBlockFlag::Cached); }
	inline bool is_volatile() const		{ return CheckEnumFlag(flags, ContextBlockFlag::Volatile); }
	inline bool is_temporary() const	{ return ttl > 0; }

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

using SequenceList = std::vector<int32_t>;

struct ContextSequence
{
	llama_context* pCtx = nullptr;
	int32_t seq_index = 0;						// Sequence index
	std::vector<llama_seq_id> seq_ids { 0 };
	llama_batch batch {};						// Representation of the kv-cache (mirror)
	std::vector<ContextBlock> blocks {};
	int32_t response_pos = 0;					// start of response, including prompt template preamble
	int32_t prepend_pos = 0;					// start of response, not including prompt template preamble
	int32_t cursor_pos = 0;						// current position
	
	void AssignBlockPositions();
	int32_t GetFirstNonStaticOffset() const;
	int32_t RemoveAndShift(const llama_vocab* pVocab, std::vector<ContextBlock>::iterator itBegin, std::vector<ContextBlock>::iterator itEnd);
	bool RebuildKVCache();

	int32_t EraseTokens(int32_t from, int32_t length);
	int32_t ShiftTokens(int32_t pos, int32_t len, int32_t offset);
	int32_t BatchWrite(const std::vector<llama_token>& tokens, int32_t pos);
	int32_t BatchRemove(int32_t begin, int32_t end);
	int32_t BatchAllocate(int32_t pos, int32_t length);
	void BatchSetSequences(int32_t pos, const std::vector<int32_t>& seqIds);
};

struct ContextState
{
	llama_context* pCtx = nullptr;
	const llama_vocab* pVocab = nullptr;

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

	int32_t get_max_position() const;
};