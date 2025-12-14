#pragma once

#include "llm/LLMTypes.h"
#include "llm/Context.h"

#include <optional>

namespace llm_util
{
	fig::string stringFromToken(fig::VocabPtr pVocab, fig::Token token);
	size_t string_find_partial_stop(const fig::string& str, const fig::string& stop);
	size_t find_one_of(const fig::string& text, const std::vector<fig::string>& words);
	size_t find_stopping_strings(const fig::string& text, const std::vector<fig::string>& stop_words, const size_t last_token_size, bool is_full_stop);
	void get_tag_and_name(const fig::string& text, fig::string& tag, fig::string& name);
	fig::string& sanitize_response(fig::string& text);
	fig::string& complete_message(fig::string& text);
	void process(fig::string& partial, fig::string str_token, bool* bWait, bool* bHalt, fig::string& stop_word);
	std::pair<MessageType, bool> detect_message_type(fig::string text) noexcept;
	
	llama_batch init_batch(int32_t ctx_size, int32_t n_seq_max);
	void free_batch(llama_batch& batch);

	bool init_embedding_batch(llama_model* pModel, llama_context* pCtx, const std::vector<fig::Token>& tokens, llama_batch& out_pBatch);
	llama_batch create_batch(std::span<fig::Token> tokens, std::span<llama_seq_id> seqs, int32_t n_seq_max, int32_t position);
	llama_batch create_batch_view(const llama_batch& batch, int32_t position, int32_t length);

	std::vector<fig::Token> tokenize(fig::VocabPtr pModel, fig::string prompt, bool add_special = false);
	std::vector<fig::Token> tokenize_and_batch(Context& context, fig::string content, fig::SequenceId seq_id, int32_t pos, bool add_special = false);
	std::optional<std::vector<fig::Token>> tokenize_and_decode(Context& context, fig::string content, fig::SequenceId seq_id, int32_t pos, bool add_special = false);
	void erase_bottom(llama_context* pCtx, int32_t n_max_seq, int32_t pos);

	fig::string process_message(fig::string message, fig::string actorName, std::vector<Submessage>* out_pSubmessages = nullptr) noexcept;

	fig::string format_id(fig::string id);
	bool dump_batch_text(const Context& context, int32_t seq_id, fig::string filename);
	bool dump_batch_tokens(const Context& context, int32_t seq_id, fig::string filename);
	bool dump_kv_cache(const Context& context, int32_t seq_id, fig::string filename);
	bool dump_kv_cache_cells(const Context& contextState, fig::string filename);
	
	bool dump_kv_cache_cells(llama_context* pCtx, int32_t num_sequences, fig::string filename);
	bool dump_batch_tokens(const llama_batch& batch, int32_t num_tokens, int32_t seq_id, fig::VocabPtr pVocab, fig::string filename);

	inline fig::SequenceIndices get_sequence_indices(fig::Sequence seq, int32_t n_seq_max) noexcept;
	fig::SequenceIndices get_sequence_indices(fig::SequenceId seq, int32_t n_seq_max) noexcept;
	fig::SequenceId sequence_from_index(int32_t seq_idx) noexcept;
}
