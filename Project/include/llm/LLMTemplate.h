#pragma once

#include "llm/LLMTypes.h"

enum class PromptTemplate
{
	Undefined,
    ChatML,
    Llama2_v1,
    Llama2_sys,
    Llama2_sys_bos,
    Llama2_sys_strip,
	Llama2 = Llama2_sys_bos,
    Llama3,
    Llama4,
    Deepseek,
    Deepseek2,
    Deepseek3,
    Gemma,
    MistralV1,
    MistralV3,
    MistralV3_tekken,
    MistralV7,
    Phi3,
    Phi4,
    CommandR,

	/* Unsupported 
    Falcon3,
    Zephyr,
    Monarch,
    Orion,
    OpenChat,
    Vicuna,
    VicunaOrca,
    Chatglm3,
    Chatglm4,
    Glmedge,
    Minicpm,
    Exaone3,
    Rwkv_world,
    Granite,
    Gigachat,
    Megrez,
    Yandex,
    Bailing,
    Smolvlm,
	*/
	Automatic,
	Default = Gemma
};

class llm_tmpl
{
public:
	static string apply_chat_template(llama_context* pCtx, Message msg, bool add_assistant);
	static string apply_chat_template_prefix(llama_context* pCtx, Message msg, string name);
	static std::pair<string, string> get_chat_template_prefix_suffix(llama_context* pCtx, Role role, string name);
	static std::string get_responder_prelude(Responder responder, llama_context* pCtx) noexcept;
	static PromptTemplate auto_detect_template(llama_model* pModel);

	static PromptTemplate current_template;

private:
	static string _template;
};