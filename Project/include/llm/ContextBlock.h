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

	llama_seq_id get_any_sequence_id() const noexcept;
	[[nodiscard]] SequenceIndices get_sequence_ids(int32_t n_seq_max) const noexcept;
};
