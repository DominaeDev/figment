#pragma once

#include "llm/LLMTypes.h"
#include "model/ChatTypes.h"

namespace fig::llm
{
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
		static fig::string apply_chat_template(Messages msg, bool add_assistant);

		static std::pair<fig::string, fig::string> get_chat_template_prefix_suffix(Role role, fig::string name);
		static fig::string apply_chat_template_prefix(Role role, fig::string content, fig::string name);

		static PromptTemplate current_template;

	private:
		static fig::string _template;
	};
}