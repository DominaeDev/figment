#ifndef PROMPT_TEMPLATE_TYPES_H__
#define PROMPT_TEMPLATE_TYPES_H__
#pragma once

namespace fig::llm
{
	enum class PromptTemplateType
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
		
		// Falcon3,
		// Zephyr,
		// Monarch,
		// Orion,
		// OpenChat,
		// Vicuna,
		// VicunaOrca,
		// Chatglm3,
		// Chatglm4,
		// Glmedge,
		// Minicpm,
		// Exaone3,
		// Rwkv_world,
		// Granite,
		// Gigachat,
		// Megrez,
		// Yandex,
		// Bailing,
		// Smolvlm,
		
		Automatic,
		Default = ChatML
	};
}
#endif