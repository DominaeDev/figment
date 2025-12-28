#include "llm/LLMTypes.h"
#include "model/ChatTypes.h"

#pragma once

namespace fig::llm
{
	enum class ContextBlockFlag : uint32_t
	{
		Cached = 1 << 0,		// Currently resides in the kv-cache
		
		Static = 1 << 1,		// Static content
		Persona = 1 << 2,		// Persona content

		Volatile = 1 << 3,		// Should discard immediately
		Discard = 1 << 4,		// Tagged for removal
		Contination = 1 << 5,	// Incomplete prompt template
	};
	using ContextBlockFlags = EnumFlags<ContextBlockFlag>;

	struct ContextBlock
	{
		Role role = Role::Undefined;
		fig::string name;
		fig::string content;
		std::vector<int32_t> tokens;
		ContextBlockFlags flags {};
		int32_t attn_position = -1;
		int32_t cache_position = -1;
		SequenceSlots sequenceSlots = SequenceSlots::None;
		int turn = -1;
		int ttl = 0;
		fig::string responseId;

		inline int32_t length() const { return (int32_t)(tokens.size()); }
		inline bool is_static() const { return flags.IsSet(ContextBlockFlag::Static); }
		inline bool is_cached() const { return flags.IsSet(ContextBlockFlag::Cached); }
		inline bool is_volatile() const { return flags.IsSet(ContextBlockFlag::Volatile); }
		inline bool is_discarded() const { return flags.IsAnySet({ ContextBlockFlag::Discard, ContextBlockFlag::Volatile }); }
		inline bool is_temporary() const { return ttl > 0; }
		inline bool is_persona() const { return flags.IsSet(ContextBlockFlag::Persona); }
		inline bool is_continuation() const { return flags.IsSet(ContextBlockFlag::Contination); }

		Sequence get_any_sequence_id() const noexcept;
		[[nodiscard]] Sequences get_sequence_ids(int32_t n_seq_max) const noexcept;

		inline void Discard() { flags.Set(ContextBlockFlag::Discard); }

		bool operator==(const ContextBlock& other) const = default;
	};
}