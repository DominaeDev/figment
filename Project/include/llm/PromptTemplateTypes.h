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

	const std::map<PromptTemplateType, fig::string> PromptTemplateMapping {
		{ PromptTemplateType::ChatML, "ChatML" },
		{ PromptTemplateType::Llama2_v1, "Llama2_v1" },
		{ PromptTemplateType::Llama2_sys, "Llama2_sys" },
		{ PromptTemplateType::Llama2_sys_bos, "Llama2_sys_bos" },
		{ PromptTemplateType::Llama2_sys_strip, "Llama2_sys_strip" },
		{ PromptTemplateType::Llama3, "Llama3" },
		{ PromptTemplateType::Llama4, "Llama4" },
		{ PromptTemplateType::Deepseek, "Deepseek" },
		{ PromptTemplateType::Deepseek2, "Deepseek2" },
		{ PromptTemplateType::Deepseek3, "Deepseek3" },
		{ PromptTemplateType::Gemma, "Gemma" },
		{ PromptTemplateType::MistralV1, "MistralV1" },
		{ PromptTemplateType::MistralV3, "MistralV3" },
		{ PromptTemplateType::MistralV3_tekken, "MistralV3_tekken" },
		{ PromptTemplateType::MistralV7, "MistralV7" },
		{ PromptTemplateType::Phi3, "Phi3" },
		{ PromptTemplateType::Phi4, "Phi4" },
		{ PromptTemplateType::CommandR, "CommandR" },
	};
}
