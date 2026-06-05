#pragma once

#include "llm/LLMTypes.h"
#include "llm/LLMContext.h"

#include <optional>

namespace fig::llm
{
	size_t string_find_partial_stop(const fig::string& str, const fig::string& stop);
	size_t find_one_of(const fig::string& text, const std::vector<fig::string>& words);
	size_t find_stopping_strings(const fig::string& text, const std::vector<fig::string>& stop_words, const size_t last_token_size, bool is_full_stop);
	void get_tag_and_name(const fig::string& text, fig::string& tag, fig::string& name);
	fig::string& sanitize_response(fig::string& text);
	fig::string& complete_message(fig::string& text);
	
	void process(fig::string& partial, fig::string str_token, bool* bWait, bool* bHalt, fig::string& stop_word);
	fig::string process_message(fig::string message, fig::string actorName, std::vector<fig::chat::Submessage>* out_pSubmessages = nullptr) noexcept;
	std::pair<fig::chat::MessageType, bool> detect_message_type(fig::string text) noexcept;

	bool dump_batch_text(const LLMContext& context, int32_t seq_id, const fig::path& filename);
	bool dump_batch_tokens(const LLMContext& context, int32_t seq_id, const fig::path& filename);
	bool dump_batch_tokens(const llama_batch& batch, int32_t num_tokens, int32_t seq_id, VocabPtr pVocab, const fig::path& filename);
	bool dump_kv_cache(const LLMContext& context, int32_t seq_id, const fig::path& filename);
	bool dump_kv_cache_cells(const LLMContext& contextState, const fig::path& filename);
	bool dump_kv_cache_cells(ContextPtr pCtx, int32_t num_sequences, const fig::path& filename);
	
	bool validate_kv_cache(const LLMContext& context, Sequence sequence, int32_t turn);

	inline Sequences get_sequence_indices(SequenceSlot seq, int32_t n_seq_max) noexcept;
	Sequences get_sequence_indices(SequenceSlots seq, int32_t n_seq_max) noexcept;
	SequenceSlots get_sequence_from_index(int32_t seq_idx) noexcept;
	SequenceSlot get_sequence_slot_from_index(Sequence seq_idx) noexcept;

	fig::string format_id(fig::string id);

	void embd_normalize(const std::vector<float>& inp, std::vector<float>& out, int n, int embd_norm);
	void embd_normalize(const float* inp, float* out, int n, int embd_norm);
	float embd_similarity_cos(const std::vector<float>& embd1, const std::vector<float>& embd2, int n);

	PromptTemplateType auto_detect_template(llama_model* pModel);
	fig::string apply_chat_template(fig::chat::Messages msg, bool add_assistant);

	std::pair<fig::string, fig::string> get_chat_template_prefix_suffix(fig::chat::Role role, fig::string name);
	fig::string apply_chat_template_prefix(fig::chat::Role role, fig::string content, fig::string name);

}
