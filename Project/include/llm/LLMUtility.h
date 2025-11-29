#pragma once

#include "llm/LLMTypes.h"
#include "llm/Context.h"
#include <optional>

namespace llm_util
{
	std::string stringFromToken(VocabPtr pVocab, llama_token token);
	size_t validate_utf8(const string& text) noexcept;
	size_t string_find_partial_stop(const std::string& str, const std::string& stop);
	size_t find_one_of(const string& text, const std::vector<string>& words);
	size_t find_stopping_strings(const string& text, const std::vector<string>& stop_words, const size_t last_token_size, bool is_full_stop);
	void get_tag_and_name(const string& text, string& tag, string& name);
	string& sanitize_response(string& text);
	string& complete_message(string& text);
	void process(string& partial, string str_token, bool* bWait, bool* bHalt, string& stop_word);
	std::pair<MessageType, bool> detect_message_type(string text) noexcept;
	
	llama_batch init_batch(int32_t ctx_size, int32_t n_seq_max);
	void free_batch(llama_batch& batch);

	bool init_embedding_batch(llama_model* pModel, llama_context* pCtx, const std::vector<llama_token>& tokens, llama_batch& out_pBatch);
	llama_batch create_batch(std::span<llama_token> tokens, std::span<llama_seq_id> seqs, int32_t n_seq_max, int32_t position);
	llama_batch create_batch_view(const llama_batch& batch, int32_t position, int32_t length);

	std::vector<llama_token> tokenize(VocabPtr pModel, string prompt, bool add_special = false);
	std::vector<llama_token> tokenize_and_batch(VocabPtr pModel, ContextSequence& seq, string content, SequenceId seq_id, int32_t pos, bool add_special = false);
	std::optional<std::vector<llama_token>> tokenize_and_decode(VocabPtr pModel, ContextSequence& seq, string content, SequenceId seq_id, int32_t pos, bool add_special = false);
	void erase_bottom(llama_context* pCtx, int32_t n_max_seq, int32_t pos);

	std::string process_message(std::string message, std::string actorName, std::vector<Submessage>* out_pSubmessages = nullptr) noexcept;

	string format_id(string id);
	bool dump_batch_text(const ContextSequence& seq, int32_t seq_id, VocabPtr pVocab, string filename);
	bool dump_batch_tokens(const ContextSequence& seq, int32_t seq_id, VocabPtr pVocab, string filename);
	bool dump_batch_tokens(const llama_batch& batch, int32_t num_tokens, int32_t seq_id, VocabPtr pVocab, string filename);
	bool dump_kv_cache(const ContextSequence& seq, int32_t seq_id, string filename);
	bool dump_kv_cache_cells(llama_context* pCtx, int32_t num_sequences, string filename);
	bool dump_kv_cache_cells(const ContextState& contextState, string filename);
	llama_sampler* compile_grammar(GrammarFlag flags, VocabPtr pVocab, string names, string stateVars);

	SequenceIndices get_sequence_indices(SequenceId seq, int32_t n_seq_max) noexcept;
	constexpr SequenceId sequence_from_index(int32_t seq_idx) noexcept;
}
