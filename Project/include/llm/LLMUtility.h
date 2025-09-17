#pragma once

#include "llm/LLMTypes.h"
#include <llama.h>
#include <optional>

namespace llm_util
{
	std::string stringFromToken(const llama_vocab* pVocab, llama_token token);
	size_t validate_utf8(const string& text) noexcept;
	size_t string_find_partial_stop(const std::string& str, const std::string& stop);
	size_t find_one_of(const string& text, const std::vector<string>& words);
	size_t find_stopping_strings(const string& text, const std::vector<string>& stop_words, const size_t last_token_size, bool is_full_stop);
	void get_tag_and_name(const string& text, string& tag, string& name);
	string& sanitize_response(string& text);
	string& complete_message(string& text);
	void process(string& partial, string str_token, bool* bWait, bool* bHalt, string& stop_word);
	
	llama_batch init_batch(llama_context* pCtx);
	bool init_batch(llama_model* pModel, llama_context* pCtx, string prompt, llama_batch& out_pBatch);
	void init_batch_logits(llama_batch& batch);
	bool init_embedding_batch(llama_model* pModel, llama_context* pCtx, const std::vector<llama_token>& tokens, llama_batch& out_pBatch);
	llama_batch create_batch_view(const llama_batch& batch, int32_t position, int32_t length);
	std::vector<llama_token> tokenize(llama_model* pModel, string prompt, bool add_special = false);
	std::vector<llama_token> tokenize_and_batch(llama_model* pModel, llama_context* pCtx, llama_batch& batch, string content, int32_t pos, bool add_special = false);
	std::optional<std::vector<llama_token>> tokenize_and_decode(llama_model* pModel, llama_context* pCtx, llama_batch& batch, string content, int32_t pos, bool add_special = false);
	int32_t erase_tokens(llama_context* pCtx, llama_batch& batch, int32_t from, int32_t length);
	int32_t shift_tokens(llama_context* pCtx, llama_batch& batch, int32_t pos, int32_t len, int32_t shift_amount);
	int32_t batch_write(llama_model* pModel, llama_context* pCtx, llama_batch& batch, const std::vector<llama_token>& tokens, int32_t pos);
	int32_t batch_remove(llama_context* pCtx, llama_batch& batch, int32_t begin, int32_t end);
	int32_t batch_allocate(llama_context* pCtx, llama_batch& batch, int32_t begin, int32_t length);
	int32_t ctx_remove_and_shift(llama_model* pModel, llama_context* pCtx, ContextSequence& seq, std::vector<ContextBlock>::iterator itBegin, std::vector<ContextBlock>::iterator itEnd);

	std::pair<MessageType, bool> detect_message_type(string text) noexcept;

	std::string process_message(std::string message, std::string actorName, std::vector<Submessage>* out_pSubmessages = nullptr) noexcept;

	string format_id(string id);
	bool dump_context(const llama_batch& batch, const llama_vocab* pVocab, string filename);
	llama_sampler* compile_grammar(GrammarFlag flags, const llama_vocab* pVocab, string names, string stateVars);
}
