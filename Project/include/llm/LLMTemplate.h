#pragma once

#include "llm/LLMTypes.h"
#include "model/ChatTypes.h"

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

/*	Not supported for now: */
/*	
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
	Default = ChatML
};

class llm_tmpl
{
public:
	static PromptTemplate auto_detect_template(llama_model* pModel);
	static string apply_chat_template(Messages msg, bool add_assistant);
	
	static std::pair<string, string> get_chat_template_prefix_suffix(Role role, string name);
	static string apply_chat_template_prefix(Role role, string content, string name);

	static PromptTemplate current_template;

private:
	static string _template;
};