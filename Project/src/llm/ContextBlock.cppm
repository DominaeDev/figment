export module ContextBlock;

import Types;
import Utility;
import LLMTypes;
import Constants;
import <cassert>;

export 
{
	enum class ContextBlockFlag : uint32_t
	{
		Static = 1 << 0,	// Static instruction
		Cached = 1 << 1,	// Is currently in kv-cache
		Volatile = 1 << 2,	// Should be discarded immediately
		Persona = 1 << 3,	// Persona
	};
	using ContextBlockFlags = EnumFlags<ContextBlockFlag>;

	struct ContextBlock
	{
		Role role = Role::Undefined;
		string name;
		string content;
		std::vector<int32_t> tokens;
		ContextBlockFlags flags {};
		SequenceId sequenceId = SequenceId::None;
		int32_t offset = 0;
		int ttl = 0;
		string responseId;

		inline int32_t length() const { return (int32_t)(tokens.size()); }
		inline bool is_static() const { return flags.IsSet(ContextBlockFlag::Static); }
		inline bool is_cached() const { return flags.IsSet(ContextBlockFlag::Cached); }
		inline bool is_volatile() const { return flags.IsSet(ContextBlockFlag::Volatile); }
		inline bool is_temporary() const { return ttl > 0; }
		llama_seq_id get_any_sequence_id() const noexcept;
		[[nodiscard]] SequenceIndices get_sequence_ids(int32_t n_seq_max) const noexcept;
	};
}

llama_seq_id ContextBlock::get_any_sequence_id() const noexcept
{
	for (size_t i = 0; i < Constants::Context::AllSequenceIDs.size() && i < Constants::Context::MaxSequences; ++i)
	{
		if (sequenceId.IsSet(Constants::Context::AllSequenceIDs[i]))
			return static_cast<llama_seq_id>(i);
	}
	assert(0 && "Block has no sequence");
	return -1;
}

SequenceIndices ContextBlock::get_sequence_ids(int32_t n_seq_max) const noexcept
{
	SequenceIndices seqIds;
	seqIds.reserve(n_seq_max);

	for (size_t i = 0; i < Constants::Context::AllSequenceIDs.size() && i < n_seq_max; ++i)
	{
		if (sequenceId.IsSet(Constants::Context::AllSequenceIDs[i]))
			seqIds.push_back(toI(i));
	}
	return seqIds;
}