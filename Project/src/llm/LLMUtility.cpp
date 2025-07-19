#include "llm/LLMUtility.h"
#include "util/StringUtility.h"
#include "Constants.h"
#include <format>

static std::vector<string> const opening_tags {
	std::format("<{0}=\"", Constants::DialogueTag),
	std::format("<{0}=\"", Constants::ActionTag),
	std::format("<{0}=\"", Constants::ThoughtTag),
	std::format("<{0}>", Constants::NarrationTag),
	std::format("<{0}>", Constants::DirectionTag),
};


static std::vector<string> const closing_tags {
	std::format("</{0}>", Constants::DialogueTag),
	std::format("</{0}>", Constants::ActionTag),
	std::format("</{0}>", Constants::ThoughtTag),
	std::format("</{0}>", Constants::NarrationTag),
	std::format("</{0}>", Constants::DirectionTag),
};

std::string llm_util::stringFromToken(const llama_vocab* pVocab, llama_token token)
{
	// convert the token to a string, print it and add it to the response
	char buf[256];
	int n = llama_token_to_piece(pVocab, token, buf, sizeof(buf), 0, false);
	if (n < 0)
		return "";

	return std::string(buf, n);
}

size_t llm_util::validate_utf8(const string& text)
{
	size_t len = text.size();
	if (len == 0) return 0;

	// Check the last few bytes to see if a multi-byte character is cut off
	for (size_t i = 1; i <= 4 && i <= len; ++i)
	{
		unsigned char c = text[len - i];
		// Check for start of a multi-byte sequence from the end
		if ((c & 0xE0) == 0xC0)
		{
			// 2-byte character start: 110xxxxx
			// Needs at least 2 bytes
			if (i < 2) return len - i;
		}
		else if ((c & 0xF0) == 0xE0)
		{
			// 3-byte character start: 1110xxxx
			// Needs at least 3 bytes
			if (i < 3) return len - i;
		}
		else if ((c & 0xF8) == 0xF0)
		{
			// 4-byte character start: 11110xxx
			// Needs at least 4 bytes
			if (i < 4) return len - i;
		}
	}

	// If no cut-off multi-byte character is found, return full length
	return len;
}

size_t llm_util::string_find_partial_stop(const std::string_view& str, const std::string_view& stop)
{
	if (!str.empty() && !stop.empty())
	{
		const char text_last_char = str.back();
		for (int64_t char_index = stop.size() - 1; char_index >= 0; char_index--)
		{
			if (stop[char_index] == text_last_char)
			{
				const auto current_partial = stop.substr(0, char_index + 1);
				if (string_util::string_ends_with(str, current_partial))
				{
					return str.size() - char_index - 1;
				}
			}
		}
	}

	return string::npos;
}

size_t llm_util::find_one_of(const string& text, const std::vector<string>& words)
{
	size_t stop_pos = string::npos;

	for (const string& word : words)
	{
		size_t pos = text.find(word);
		if (pos != string::npos && (stop_pos == string::npos || pos < stop_pos))
			stop_pos = pos;
	}

	return stop_pos;
}

size_t llm_util::find_stopping_strings(const string& text, const std::vector<string>& stop_words, const size_t last_token_size, bool is_full_stop)
{
	size_t stop_pos = string::npos;

	for (const string& word : stop_words)
	{
		size_t pos;

		if (is_full_stop)
		{
			const size_t tmp = word.size() + last_token_size;
			const size_t from_pos = text.size() > tmp ? text.size() - tmp : 0;

			pos = text.find(word, from_pos);
		}
		else
		{
			// otherwise, partial stop
			pos = string_find_partial_stop(text, word);
		}

		if (pos != string::npos && (stop_pos == string::npos || pos < stop_pos))
		{
			stop_pos = pos;
		}
	}

	return stop_pos;
}

void llm_util::get_tag_and_name(const string& text, string& tag, string& name)
{
	size_t pos_equals = text.find('=', 1);
	if (pos_equals == string::npos)
	{
		tag = string_util::trim(text.substr(1, text.length() - 2));
		name = "";
		return;
	}

	tag = string_util::trim(text.substr(1, pos_equals - 1));
	name = string_util::trim(text.substr(pos_equals + 1, text.length() - pos_equals - 2));
	string_util::replace_all(name, "\"", "");
}

void llm_util::apply_names(string& prompt, string userName, string botName)
{
	string_util::replace_all(prompt, "{{user}}", userName);
	string_util::replace_all(prompt, "{{char}}", botName);
}

const char* llm_util::name_from_role(Role role)
{
	static const char* SYSTEM_NAME = "system";
	static const char* NARRATOR_NAME = "Narrator";
	static const char* DIRECTOR_NAME = "Director";
	static const char* USER_NAME = "{{user}}";
	static const char* BOT_NAME = "{{char}}";

	switch (role)
	{
	case Role::Bot: return BOT_NAME;
	case Role::User: return USER_NAME;
	case Role::System: return SYSTEM_NAME;
	case Role::Narrator: return NARRATOR_NAME;
	case Role::Director: return DIRECTOR_NAME;
	}
	return "";
}

string llm_util::apply_chat_template(Messages in_messages, llama_context* pCtx, bool add_assistant)
{
	int prev_len = 0;

//	const char* tmpl = llama_model_chat_template(state.pModel, nullptr);
	const char* tmpl = "chatml";

//	tmpl = "mistral-v7-tekken";
//	tmpl = "chatml";
//	tmpl = "llama2";
//	tmpl = "llama3";
//	tmpl = "command-r";
//	tmpl = "gemma";
//	tmpl = "vicuna";
//	tmpl = "deepseek3";

	std::vector<llama_chat_message> llama_msgs(in_messages.size());
	for (int i = 0; i < in_messages.size(); ++i)
	{
		auto& msg = in_messages[i];
		llama_msgs[i] = llama_chat_message { 
			msg.name.empty() ? name_from_role(msg.role) : msg.name.c_str(), 
			msg.content.c_str() 
		};
	}

	std::vector<char> formatted(llama_n_ctx(pCtx));
	int new_len = llama_chat_apply_template(tmpl, llama_msgs.data(), (int32_t)llama_msgs.size(), add_assistant, formatted.data(), (int32_t)formatted.size());
	if (new_len > (int)formatted.size())
	{
		formatted.resize(new_len);
		new_len = llama_chat_apply_template(tmpl, llama_msgs.data(), (int32_t)llama_msgs.size(), add_assistant, formatted.data(), (int32_t)formatted.size());
	}

	if (new_len < 0)
	{
		fprintf(stderr, "failed to apply the chat template\n");
		return "";
	}

	// remove previous messages to obtain the prompt to generate the response
	string prompt = string(formatted.begin() + prev_len, formatted.begin() + new_len);

	return prompt;
}

string llm_util::apply_chat_template(Message msg, llama_context* pCtx, bool add_assistant)
{
	return apply_chat_template(Messages { msg }, pCtx, add_assistant);
}

string llm_util::apply_chat_template_prefix(Message msg, string userName, string botName, llama_context* pCtx, bool add_assistant)
{
	// Strip prompt template from block content
	static const char* const MARKER = "{{__PLACEHOLDER__}}";
	string tmpl = apply_chat_template(Message { msg.role, MARKER }, pCtx, false);
	size_t pos_msg = tmpl.find(MARKER);
	string prelude = tmpl.substr(0, pos_msg);
	string postlude = tmpl.substr(pos_msg + strlen(MARKER));
	apply_names(prelude, userName, botName);

	string content = msg.content;
	if (string_util::string_ends_with(content, postlude))
		content = content.substr(0, content.length() - postlude.length());
	if (!string_util::string_begins_with(content, prelude))
		content = prelude + content;
	return content;
}

std::pair<MessageType, bool> llm_util::detect_message_type(string text)
{
	auto patterns = std::vector<std::tuple<string, MessageType>> {
		{ std::format("<{0}", Constants::DialogueTag),		MessageType::Dialogue},
		{ std::format("<{0}", Constants::ActionTag),		MessageType::Action},
		{ std::format("<{0}", Constants::ThoughtTag),		MessageType::Thought},
		{ std::format("<{0}", Constants::NarrationTag),		MessageType::Narration},
		{ std::format("<{0}", Constants::DirectionTag),		MessageType::Direction},
	};
	
	size_t last_pos = std::string::npos;
	MessageType msgType = MessageType::Undefined;
	for (int i = 0; i < patterns.size(); ++i)
	{
		size_t pos = text.rfind(std::get<0>(patterns[i]), std::string::npos);
		if (pos != std::string::npos && (last_pos == std::string::npos || pos > last_pos))
		{
			last_pos = pos;
			msgType = std::get<1>(patterns[i]);
		}
	}

	bool bComplete = false;
	for (auto tag : closing_tags)
	{
		size_t pos_end = text.rfind(tag, std::string::npos);
		if (pos_end != std::string::npos && pos_end > last_pos)
		{
			bComplete = true;
			break;
		}
	}

	return std::make_pair(msgType, bComplete);
}

string& llm_util::sanitize_response(string& text)
{
	size_t pos_last = text.find_last_of('<');
	if (pos_last == std::string::npos)
		return text;
	size_t pos_close = text.find_last_of('>');
	if (pos_close == std::string::npos || pos_close < pos_last)
		text = text.substr(0, pos_last);
	return text;
}

string& llm_util::complete_message(string& text)
{
	auto [msgType, complete] = detect_message_type(sanitize_response(text));
	if (complete)
		return text;

	switch (msgType)
	{
	case MessageType::Undefined:
		text = std::format("<{0}=\"{{{{char}}}}\">{1}</{0}>", Constants::DialogueTag, text);
		break;
	case MessageType::Dialogue:
		text.append(std::format("\"</{0}>", Constants::DialogueTag));
		break;
	case MessageType::Action:
		text.append(std::format("*</{0}>", Constants::ActionTag));
		break;
	case MessageType::Thought:
		text.append(std::format("))</{0}>", Constants::ThoughtTag));
		break;
	case MessageType::Narration:
		text.append(std::format("]</{0}>", Constants::NarrationTag));
		break;
	case MessageType::Direction:
		text.append(std::format("</{0}>", Constants::DirectionTag));
		break;
	case MessageType::SystemMessage:
	default:
		break;
	}
	return text;
}

void llm_util::process(string& partial, string str_token, bool* bWait, bool* bHalt, string& stop_word)
{
	if (validate_utf8(partial) < partial.size()) // Incomplete utf-8 string
	{
		*bWait = true;
		*bHalt = false;
		return;
	}

	static std::vector<string> stop_words {
		"<|",
		"<end_of_turn",
		"<EOT>",
		"_<EOT>",
		"<s>",
		"</s>",
		"### ",
		"<｜",
		"\r\r",
//		"<|end",
//		"<｜end▁of▁sentence｜>",
	};

	static std::vector<string> formatting_tags;
	if (formatting_tags.empty())
	{
		formatting_tags.insert(std::end(formatting_tags), std::begin(opening_tags), std::end(opening_tags));
		formatting_tags.insert(std::end(formatting_tags), std::begin(closing_tags), std::end(closing_tags));
	}

	// Look for stop word - and halt
	size_t stop_pos = find_stopping_strings(partial, stop_words, str_token.size(), true);
	if (stop_pos != string::npos)
	{
		stop_word = partial.substr(stop_pos);

		// Print to console
		partial = partial.erase(stop_pos); // Erase stop word
		*bHalt = true;
		*bWait = false;
		return;
	}

	// Look for partial stop word - and wait
	stop_pos = find_stopping_strings(partial, stop_words, str_token.size(), false);
	if (stop_pos != string::npos)
	{
		*bHalt = false;
		*bWait = true;
		return;
	}

	// Look for formatting tags
	size_t fmt_pos = find_one_of(partial, opening_tags);
	if (fmt_pos != string::npos)
	{
		// Await end of tag '>', or beginning of a new tag '<' (indicating garbage from the model)
		if (partial.find_first_of("<>", fmt_pos + 1, 2) == string::npos)
		{
			*bHalt = false;
			*bWait = true;
			return;
		}
	}
	else
	{
		// Look for partial formatting tags - and wait
		fmt_pos = find_stopping_strings(partial, formatting_tags, str_token.size(), false);
		if (fmt_pos != string::npos)
		{
			*bHalt = false;
			*bWait = true;
			return;
		}
	}

	*bHalt = false;
	*bWait = false;
}

void llm_util::init_batch_logits(llama_batch& batch)
{
	if (batch.n_tokens <= 0)
		return;

	for (int i = 0; i < batch.n_tokens - 1; ++i)
		batch.logits[i] = false;
	batch.logits[batch.n_tokens - 1] = true;  // Only need logits for last token
}

std::vector<llama_token> llm_util::tokenize(llama_model* pModel, string prompt, bool bAddSpecial)
{
	const llama_vocab* pVocab = llama_model_get_vocab(pModel);

	std::vector<llama_token> prompt_tokens(1024);
	const int32_t n_prompt_tokens = llama_tokenize(pVocab, prompt.c_str(), (int32_t)prompt.size(), prompt_tokens.data(), (int32_t)prompt_tokens.size(), bAddSpecial, true);
	if (n_prompt_tokens < 0)
	{
		prompt_tokens.resize(-n_prompt_tokens);
		if (llama_tokenize(pVocab, prompt.c_str(), (int32_t)prompt.size(), prompt_tokens.data(), (int32_t)prompt_tokens.size(), bAddSpecial, true) < 0)
		{
			// Error
			return std::vector<llama_token> {};
		}
	}
	else
	{
		prompt_tokens.resize(n_prompt_tokens);
	}
	return prompt_tokens;
}

bool llm_util::init_batch(llama_model* pModel, llama_context* pCtx, string prompt, llama_batch& out_pBatch)
{
	const int32_t maxCtx = llama_n_ctx(pCtx);
	const bool is_first = llama_kv_self_used_cells(pCtx) == 0;

	// tokenize the prompt
	std::vector<llama_token> prompt_tokens = tokenize(pModel, prompt, is_first);

	// Prepare a batch for the prompt
	llama_batch batch = llama_batch_init(maxCtx, 0, 1);
	int32_t num_tokens = (int32_t)prompt_tokens.size();

	// Add tokens to batch
	for (int i = 0; i < num_tokens; ++i) {
		batch.token[i] = prompt_tokens[i];
		batch.pos[i] = i;  // Position in sequence
		batch.n_seq_id[i] = 1;  // This token belongs to 1 sequence
		batch.seq_id[i][0] = 0;  // Sequence ID 0
		batch.logits[i] = false;  // Don't need logits for most tokens
	}
	batch.logits[num_tokens - 1] = true;  // Only need logits for last token
	batch.n_tokens = num_tokens;

	out_pBatch = batch;
	return true;
}
