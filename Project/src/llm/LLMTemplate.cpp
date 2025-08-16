#include "llm/LLMTemplate.h"
#include "util/StringUtility.h"
#include "Constants.h"
#include <cassert>

PromptTemplate llm_tmpl::current_template = PromptTemplate::Default;
string llm_tmpl::_template {};

static const std::map<PromptTemplate, std::string> LLAMA_CPP_TEMPLATES = {
	{ PromptTemplate::ChatML,			"chatml",			},
	{ PromptTemplate::Llama2_v1,		"llama2",			},
	{ PromptTemplate::Llama2_sys,		"llama2-sys",		},
	{ PromptTemplate::Llama2_sys_bos,	"llama2-sys-bos",	},
	{ PromptTemplate::Llama2_sys_strip,	"llama2-sys-strip",	},
	{ PromptTemplate::Llama3,			"llama3",			},
	{ PromptTemplate::Llama4,			"llama4",			},
	{ PromptTemplate::Deepseek,			"deepseek",			},
	{ PromptTemplate::Deepseek2,		"deepseek2",		},
	{ PromptTemplate::Deepseek3,		"deepseek3",		},	
	{ PromptTemplate::Gemma,			"gemma",			},	// only: user, model

	{ PromptTemplate::MistralV1,		"mistral-v1",		},
	{ PromptTemplate::MistralV3,		"mistral-v3",		},
	{ PromptTemplate::MistralV3_tekken,	"mistral-v3-tekken",},
	{ PromptTemplate::MistralV7,		"mistral-v7",		},
	{ PromptTemplate::Phi3,				"phi3",				},
	{ PromptTemplate::Phi4,				"phi4",				},
	{ PromptTemplate::CommandR,			"command-r",		},

	/* Unsupported
	{ PromptTemplate::Vicuna,			"vicuna",			},
	{ PromptTemplate::VicunaOrca,		"vicuna-orca",		},
	{ PromptTemplate::Falcon3,			"falcon3",			},
	{ PromptTemplate::Zephyr,			"zephyr",			},
	{ PromptTemplate::Monarch,			"monarch",			},
	{ PromptTemplate::Orion,			"orion",			},
	{ PromptTemplate::OpenChat,			"openchat",			},
	{ PromptTemplate::Chatglm3,			"chatglm3",			},
	{ PromptTemplate::Chatglm4,			"chatglm4",			},
	{ PromptTemplate::Glmedge,			"glmedge",			},
	{ PromptTemplate::Minicpm,			"minicpm",			},
	{ PromptTemplate::Exaone3,			"exaone3",			},
	{ PromptTemplate::Rwkv_world,		"rwkv-world",		},
	{ PromptTemplate::Granite,			"granite",			},
	{ PromptTemplate::Gigachat,			"gigachat",			},
	{ PromptTemplate::Megrez,			"megrez",			},
	{ PromptTemplate::Yandex,			"yandex",			},
	{ PromptTemplate::Bailing,			"bailing",			},
	{ PromptTemplate::Smolvlm,			"smolvlm",			},
	*/
};

#define LU8(x) (const char*)(u8##x)

static const char* get_tmpl(PromptTemplate tmpl)
{
	return LLAMA_CPP_TEMPLATES.find(tmpl)->second.c_str();
}

// Lifted from llama.cpp API
static int32_t apply_template(PromptTemplate tmpl, const std::vector<const llama_chat_message*>& chat, std::string& dest, bool add_ass) 
{
    // Taken from the research: https://github.com/ggerganov/llama.cpp/issues/5527
    std::stringstream ss;
    if (tmpl == PromptTemplate::ChatML) {
        // chatml template
        for (auto message : chat) {
            ss << "<|im_start|>" << message->role << "\n" << message->content << "<|im_end|>\n";
        }
        if (add_ass) {
            ss << "<|im_start|>assistant\n";
        }
    } else if (tmpl == PromptTemplate::MistralV7) {
        // Official mistral 'v7' template
        // See: https://huggingface.co/mistralai/Mistral-Large-Instruct-2411#basic-instruct-template-v7
        for (auto message : chat) {
            std::string role(message->role);
            std::string content(message->content);
            if (role == "system") {
                ss << "[SYSTEM_PROMPT] " << content << "[/SYSTEM_PROMPT]";
            } else if (role == "user") {
                ss << "[INST] " << content << "[/INST]";
            }
            else {
                ss << " " << content << "</s>";
            }
        }
    } else if (tmpl == PromptTemplate::MistralV1
            || tmpl == PromptTemplate::MistralV3
            || tmpl == PromptTemplate::MistralV3_tekken) {
        // See: https://github.com/mistralai/cookbook/blob/main/concept-deep-dive/tokenization/chat_templates.md
        // See: https://github.com/mistralai/cookbook/blob/main/concept-deep-dive/tokenization/templates.md
        std::string leading_space = tmpl == PromptTemplate::MistralV1 ? " " : "";
        std::string trailing_space = tmpl == PromptTemplate::MistralV3_tekken ? "" : " ";
        bool trim_assistant_message = tmpl == PromptTemplate::MistralV3;
        bool is_inside_turn = false;
        for (auto message : chat) {
            if (!is_inside_turn) {
                ss << leading_space << "[INST]" << trailing_space;
                is_inside_turn = true;
            }
            std::string role(message->role);
            std::string content(message->content);
            if (role == "system") {
                ss << content << "\n\n";
            } else if (role == "user") {
                ss << content << leading_space << "[/INST]";
            } else {
                ss << trailing_space << (trim_assistant_message ? string_util::trim(content) : content) << "</s>";
                is_inside_turn = false;
            }
        }
    } else if (
            tmpl == PromptTemplate::Llama2_v1
            || tmpl == PromptTemplate::Llama2_sys
            || tmpl == PromptTemplate::Llama2_sys_bos
            || tmpl == PromptTemplate::Llama2_sys_strip) {
        // llama2 template and its variants
        // [variant] support system message
        // See: https://huggingface.co/blog/llama2#how-to-prompt-llama-2
        bool support_system_message = tmpl != PromptTemplate::Llama2_v1;
        // [variant] add BOS inside history
        bool add_bos_inside_history = tmpl == PromptTemplate::Llama2_sys_bos;
        // [variant] trim spaces from the input message
        bool strip_message = tmpl == PromptTemplate::Llama2_sys_strip;
        // construct the prompt
        bool is_inside_turn = true; // skip BOS at the beginning
        ss << "[INST] ";
        for (auto message : chat) {
            std::string content = strip_message ? string_util::trim(message->content) : message->content;
            std::string role(message->role);
            if (!is_inside_turn) {
                is_inside_turn = true;
                ss << (add_bos_inside_history ? "<s>[INST] " : "[INST] ");
            }
            if (role == "system") {
                if (support_system_message) {
                    ss << "<<SYS>>\n" << content << "\n<</SYS>>\n\n";
                } else {
                    // if the model does not support system message, we still include it in the first message, but without <<SYS>>
                    ss << content << "\n";
                }
            } else if (role == "user") {
                ss << content << " [/INST]";
            } else {
                ss << content << "</s>";
                is_inside_turn = false;
            }
        }
    } else if (tmpl == PromptTemplate::Phi3) {
        // Phi 3
        for (auto message : chat) {
            std::string role(message->role);
            ss << "<|" << role << "|>\n" << message->content << "<|end|>\n";
        }
        if (add_ass) {
            ss << "<|assistant|>\n";
        }
    } else if (tmpl == PromptTemplate::Phi4) {
        // chatml template
        for (auto message : chat) {
            ss << "<|im_start|>" << message->role << "<|im_sep|>" << message->content << "<|im_end|>";
        }
        if (add_ass) {
            ss << "<|im_start|>assistant<|im_sep|>";
        }
    } else if (tmpl == PromptTemplate::Gemma) {
        // google/gemma-7b-it
        std::string system_prompt = "";
        for (auto message : chat) {
            std::string role(message->role);
            if (role == "system") {
                // there is no system message for gemma, but we will merge it with user prompt, so nothing is broken
				if (chat.size() > 1)
				{
					system_prompt = string_util::trim(message->content);
					continue;
				}
				else
					role = "user";
            }
            // in gemma, "assistant" is "model"
            role = role == "assistant" ? "model" : message->role;
            ss << "<start_of_turn>" << role << "\n";
            if (!system_prompt.empty() && role != "model") {
                ss << system_prompt << "\n\n";
                system_prompt = "";
            }
            ss << string_util::trim(message->content) << "<end_of_turn>\n";
        }
        if (add_ass) {
            ss << "<start_of_turn>model\n";
        }
    } else if (tmpl == PromptTemplate::Deepseek) {
        // deepseek-ai/deepseek-coder-33b-instruct
        for (auto message : chat) {
            std::string role(message->role);
            if (role == "system") {
                ss << message->content;
            } else if (role == "user") {
                ss << "### Instruction:\n" << message->content << "\n";
            } else if (role == "assistant") {
                ss << "### Response:\n" << message->content << "\n<|EOT|>\n";
            }
        }
        if (add_ass) {
            ss << "### Response:\n";
        }
    } else if (tmpl == PromptTemplate::CommandR) {
        // CohereForAI/c4ai-command-r-plus
        for (auto message : chat) {
            std::string role(message->role);
            if (role == "system") {
                ss << "<|START_OF_TURN_TOKEN|><|SYSTEM_TOKEN|>" << string_util::trim(message->content) << "<|END_OF_TURN_TOKEN|>";
            } else if (role == "user") {
                ss << "<|START_OF_TURN_TOKEN|><|USER_TOKEN|>" << string_util::trim(message->content) << "<|END_OF_TURN_TOKEN|>";
            } else if (role == "assistant") {
                ss << "<|START_OF_TURN_TOKEN|><|CHATBOT_TOKEN|>" << string_util::trim(message->content) << "<|END_OF_TURN_TOKEN|>";
            }
        }
        if (add_ass) {
            ss << "<|START_OF_TURN_TOKEN|><|CHATBOT_TOKEN|>";
        }
    } else if (tmpl == PromptTemplate::Llama3) {
        // Llama 3
        for (auto message : chat) {
            std::string role(message->role);
            ss << "<|start_header_id|>" << role << "<|end_header_id|>\n\n" << string_util::trim(message->content) << "<|eot_id|>";
        }
        if (add_ass) {
            ss << "<|start_header_id|>assistant<|end_header_id|>\n\n";
        }
    } else if (tmpl == PromptTemplate::Deepseek2) {
        // DeepSeek-V2
        for (auto message : chat) {
            std::string role(message->role);
            if (role == "system") {
                ss << message->content << "\n\n";
            } else if (role == "user") {
                ss << "User: " << message->content << "\n\n";
            } else if (role == "assistant") {
                ss << "Assistant: " << message->content << LU8("<｜end▁of▁sentence｜>");
            }
        }
        if (add_ass) {
            ss << "Assistant:";
        }
    } else if (tmpl == PromptTemplate::Deepseek3) {
        // DeepSeek-V3
        for (auto message : chat) {
            std::string role(message->role);
            if (role == "system") {
                ss << message->content << "\n\n";
            } else if (role == "user") {
                ss << LU8("<｜User｜>") << message->content;
            } else if (role == "assistant") {
                ss << LU8("<｜Assistant｜>") << message->content << LU8("<｜end▁of▁sentence｜>");
            }
        }
        if (add_ass) {
            ss << LU8("<｜Assistant｜>");
        }
    } else if (tmpl == PromptTemplate::Llama4) {
        // Llama 4
        for (auto message : chat) {
            std::string role(message->role);
            ss << "<|header_start|>" << role << "<|header_end|>\n\n" << string_util::trim(message->content) << "<|eot|>";
        }
        if (add_ass) {
            ss << "<|header_start|>assistant<|header_end|>\n\n";
        }
    } else {
        // template not supported
        return -1;
    }
    dest = ss.str();
    return toI(dest.size());
}


std::pair<string, string> llm_tmpl::get_chat_template_prefix_suffix(Role role, string name)
{
	// Strip prompt template from block content
	static const char* const SUBSTITUTE = "{{SUBSTITUTE}}";
	string tmpl = apply_chat_template({ Message { role, SUBSTITUTE, name } }, false);
	if (tmpl.empty())
		return std::make_pair("", ""); // Unknown template

	size_t pos_msg = tmpl.find(SUBSTITUTE);
	string prefix = tmpl.substr(0, pos_msg);
	string suffix = tmpl.substr(pos_msg + strlen(SUBSTITUTE));
	return std::make_pair(prefix, suffix);
}

string llm_tmpl::apply_chat_template_prefix(Role role, string content, string name)
{
	auto [pre, post] = get_chat_template_prefix_suffix(role, name);
	
	if (string_util::ends_with(content, post))
		content = content.substr(0, content.length() - post.length());
	if (!string_util::begins_with(content, pre))
		content = pre + content;
	return content;
}

PromptTemplate llm_tmpl::auto_detect_template(llama_model* pModel)
{
	const char* tmpl = llama_model_chat_template(pModel, nullptr);
	if (tmpl)
	{
		_template = string(tmpl);
		current_template = PromptTemplate::Automatic;
		return PromptTemplate::Automatic;
	}

	// Not found
	current_template = PromptTemplate::Undefined;
	return PromptTemplate::Undefined;
}

string llm_tmpl::apply_chat_template(Messages messages, bool add_assistant)
{
	if (messages.empty())
		return "";

	std::vector<llama_chat_message> msgs;
	msgs.reserve(messages.size());
	for (size_t i = 0; i < messages.size(); ++i)
	{
		const auto& msg = messages[i];
		const char* name;
		if (is_npc(msg.role))
			name = "system";
		else if (msg.role == Role::User)
			name = "user";
		else
		{
			name = msg.name.c_str();
			// name = "assistant";
		}

		msgs.push_back(llama_chat_message { 
			name,
			msg.content.c_str() 
		});
	}

	if (!_template.empty())
	{
		std::vector<char> formatted(Constants::Context::MaxResponseLength * 2);
		int new_len = llama_chat_apply_template(_template.c_str(), msgs.data(), msgs.size(), add_assistant, formatted.data(), (int32_t)formatted.size());
		if (new_len > (int)formatted.size())
		{
			formatted.resize(new_len);
			new_len = llama_chat_apply_template(_template.c_str(), msgs.data(), msgs.size(), add_assistant, formatted.data(), (int32_t)formatted.size());
		}
		if (new_len < 0)
		{
			assert(0 && "failed to apply the chat template");
			return "";
		}

		return string(formatted.begin(), formatted.begin() + new_len);
	}
	else
	{
		std::vector<const llama_chat_message*> pMsgs;
		pMsgs.reserve(messages.size());
		for (size_t i = 0; i < msgs.size(); ++i)
			pMsgs.push_back(&msgs[i]);

		PromptTemplate tmpl = current_template;
		if (tmpl == PromptTemplate::Undefined || tmpl == PromptTemplate::Automatic)
			tmpl = PromptTemplate::Default;

		std::string formatted;
		int new_len = apply_template(tmpl, pMsgs, formatted, add_assistant);

		if (new_len < 0)
		{
			assert(0 && "failed to apply the chat template");
			return "";
		}
		return formatted;
	}

}


