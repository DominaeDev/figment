#pragma once

#include "llm/LLMTypes.h"
#include <llama.h>

namespace llm_util
{
	extern std::string stringFromToken(const llama_vocab* pVocab, llama_token token);
	extern size_t validate_utf8(const string& text) noexcept;
	extern size_t string_find_partial_stop(const std::string_view& str, const std::string_view& stop);
	extern size_t find_one_of(const string& text, const std::vector<string>& words);
	extern size_t find_stopping_strings(const string& text, const std::vector<string>& stop_words, const size_t last_token_size, bool is_full_stop);
	extern void get_tag_and_name(const string& text, string& tag, string& name);
	extern void apply_names(string& prompt, string userName, string botName);
	extern const char* name_from_role(Role role);
	extern Role role_from_responder(Responder responder);
	extern string& sanitize_response(string& text);
	extern string& complete_message(string& text);
	extern void process(string& partial, string str_token, bool* bWait, bool* bHalt, string& stop_word);
	
	extern llama_batch init_batch(llama_context* pCtx);
	extern llama_batch init_batch_view(llama_context* pCtx, llama_batch& batch, int32_t begin, int32_t end, bool logit = false);
	extern bool init_batch(llama_model* pModel, llama_context* pCtx, string prompt, llama_batch& out_pBatch);
	extern void init_batch_logits(llama_batch& batch);
	extern std::vector<llama_token> tokenize(llama_model* pModel, string prompt, bool bAddSpecial);

	extern std::pair<MessageType, bool> detect_message_type(string text) noexcept;
	extern string apply_chat_template(llama_context* pCtx, Message msg, bool add_assistant);
	extern string apply_chat_template(llama_context* pCtx, Messages in_messages, bool add_assistant);
	extern string apply_chat_template_prefix(llama_context* pCtx, Message msg);

	extern std::string process_message(std::string message, std::string actorName, std::vector<Submessage>* out_pSubmessages = nullptr) noexcept;
	extern std::string get_responder_prelude(Responder responder, llama_context* pCtx) noexcept;

	extern string format_id(string id);
}
