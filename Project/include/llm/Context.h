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
	SequenceId sequenceId = SequenceId::None;
	int32_t offset = 0;
	int ttl = 0;

	inline int32_t length() const		{ return (int32_t)(tokens.size()); }
	inline bool is_static() const		{ return CheckEnumFlag(flags, ContextBlockFlag::Static); }
	inline bool is_cached() const		{ return CheckEnumFlag(flags, ContextBlockFlag::Cached); }
	inline bool is_volatile() const		{ return CheckEnumFlag(flags, ContextBlockFlag::Volatile); }
	inline bool is_temporary() const	{ return ttl > 0; }

	[[nodiscard]] SequenceIndices get_sequence_ids() const noexcept;
};

struct ContextSequence
{
	llama_context* pCtx = nullptr;
	llama_batch batch {};						// Representation of the kv-cache (mirror)
	std::vector<ContextBlock> blocks {};
	int32_t response_pos = 0;					// start of response, including prompt template preamble
	int32_t prepend_pos = 0;					// start of response, excluding prompt template preamble
	int32_t cursor_pos = 0;						// current position (read)
	
	//void AssignBlockPositions();
	int32_t GetFirstNonStaticOffset() const;
	int32_t RemoveAndShift(size_t from, size_t to);
	bool RebuildKVCache();
	int32_t AllocateKVCache(int32_t alloc_size);
	int32_t DecodeUncached(int32_t& cursor_pos);
	int32_t GetLastBlockOffset() const;

	int32_t DecrementTTL(int32_t time);
	int32_t EraseTokens(int32_t from, int32_t length);
	int32_t ShiftTokens(int32_t pos, int32_t len, int32_t offset);
	int32_t BatchWrite(std::span<llama_token> tokens, SequenceId seq_id, int32_t pos);
	int32_t BatchRemove(int32_t begin, int32_t end);
	int32_t BatchAllocate(int32_t pos, int32_t length);
	void BatchSetSequences(int32_t pos, const std::vector<int32_t>& seqIds);
	void BatchSetSequences(int32_t from, int32_t length, SequenceId seq_id);
};

struct ContextState
{
	llama_context* pCtx = nullptr;
	const llama_vocab* pVocab = nullptr;

	std::map<Role, std::vector<int32_t>> personas;
	Role activePersona = Role::Undefined;

	ContextSequence sequence;
	int32_t previous_sequence_index = 0;
	int32_t num_sequences = 1;

	bool ReserveTokens(int32_t n_tokens, bool bForce = false);

	int32_t get_max_position() const;
	[[nodiscard]] SequenceIndices get_active_sequence() const noexcept;
	[[nodiscard]] SequenceIndices get_all_sequences() const noexcept;
};