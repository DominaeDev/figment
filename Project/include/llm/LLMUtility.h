#pragma once

#include "llm/LLMTypes.h"
#include "llm/Context.h"

#include <optional>

namespace fig::llm_util
{
	size_t string_find_partial_stop(const fig::string& str, const fig::string& stop);
	size_t find_one_of(const fig::string& text, const std::vector<fig::string>& words);
	size_t find_stopping_strings(const fig::string& text, const std::vector<fig::string>& stop_words, const size_t last_token_size, bool is_full_stop);
	void get_tag_and_name(const fig::string& text, fig::string& tag, fig::string& name);
	fig::string& sanitize_response(fig::string& text);
	fig::string& complete_message(fig::string& text);
	
	std::vector<fig::llm::Token> tokenize_and_batch(fig::llm::Context& context, fig::string content, fig::llm::SequenceSlots seq_id, int32_t pos, bool add_special = false);
	std::optional<std::vector<fig::llm::Token>> tokenize_and_decode(fig::llm::Context& context, fig::string content, fig::llm::SequenceSlots seq_id, int32_t pos, bool add_special = false);

	void process(fig::string& partial, fig::string str_token, bool* bWait, bool* bHalt, fig::string& stop_word);
	fig::string process_message(fig::string message, fig::string actorName, std::vector<Submessage>* out_pSubmessages = nullptr) noexcept;
	std::pair<MessageType, bool> detect_message_type(fig::string text) noexcept;

	bool dump_batch_text(const fig::llm::Context& context, int32_t seq_id, fig::string filename);
	bool dump_batch_tokens(const fig::llm::Context& context, int32_t seq_id, fig::string filename);
	bool dump_batch_tokens(const llama_batch& batch, int32_t num_tokens, int32_t seq_id, fig::llm::VocabPtr pVocab, fig::string filename);
	bool dump_kv_cache(const fig::llm::Context& context, int32_t seq_id, fig::string filename);
	bool dump_kv_cache_cells(const fig::llm::Context& contextState, fig::string filename);	
	bool dump_kv_cache_cells(fig::llm::ContextPtr pCtx, int32_t num_sequences, fig::string filename);
	
	bool validate_kv_cache(const fig::llm::Context& context, fig::llm::Sequence sequence);

	inline fig::llm::Sequences get_sequence_indices(fig::llm::SequenceSlot seq, int32_t n_seq_max) noexcept;
	fig::llm::Sequences get_sequence_indices(fig::llm::SequenceSlots seq, int32_t n_seq_max) noexcept;
	fig::llm::SequenceSlots get_sequence_from_index(int32_t seq_idx) noexcept;
	fig::llm::SequenceSlot get_sequence_slot_from_index(fig::llm::Sequence seq_idx) noexcept;

	fig::string format_id(fig::string id);
}
