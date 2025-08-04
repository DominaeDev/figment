#include "llm/LLMUtility.h"
#include "util/StringUtility.h"
#include "Constants.h"
#include <format>
#include <cwctype>

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

size_t llm_util::validate_utf8(const string& text) noexcept
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

size_t llm_util::string_find_partial_stop(const std::string& str, const std::string& stop)
{
	if (!str.empty() && !stop.empty())
	{
		const char text_last_char = str.back();
		for (int64_t char_index = stop.size() - 1; char_index >= 0; char_index--)
		{
			if (stop[char_index] == text_last_char)
			{
				const auto current_partial = stop.substr(0, char_index + 1);
				if (string_util::ends_with(str, current_partial))
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

std::pair<MessageType, bool> llm_util::detect_message_type(string text) noexcept
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
		formatting_tags.insert(formatting_tags.end(), opening_tags.begin(), opening_tags.end());
		formatting_tags.insert(formatting_tags.end(), closing_tags.begin(), closing_tags.end());
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

std::vector<llama_token> llm_util::tokenize(llama_model* pModel, string prompt, bool add_special)
{
	const llama_vocab* pVocab = llama_model_get_vocab(pModel);

	std::vector<llama_token> prompt_tokens(1024);
	const int32_t n_prompt_tokens = llama_tokenize(pVocab, prompt.c_str(), (int32_t)prompt.size(), prompt_tokens.data(), (int32_t)prompt_tokens.size(), add_special, false);
	if (n_prompt_tokens < 0)
	{
		prompt_tokens.resize(-n_prompt_tokens);
		if (llama_tokenize(pVocab, prompt.c_str(), (int32_t)prompt.size(), prompt_tokens.data(), (int32_t)prompt_tokens.size(), add_special, false) < 0)
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

llama_batch llm_util::init_batch(llama_context* pCtx)
{
	const int32_t maxCtx = llama_n_ctx(pCtx);

	// Prepare a batch for the prompt
	llama_batch batch = llama_batch_init(maxCtx, 0, 1);
	batch.n_tokens = 0;

	for (size_t i = 0; i < Constants::ContextSize; ++i)
	{
		batch.pos[i] = (int32_t)i;
		batch.token[i] = 0;
		batch.n_seq_id[i] = 0;
	}
	return batch;
}

llama_batch llm_util::create_batch_view(llama_batch& batch, int32_t begin, int32_t end)
{
	int32_t n_tokens = end - begin;
	return llama_batch {
		n_tokens,
		batch.token + begin,
		nullptr,
		batch.pos + begin,
		batch.n_seq_id + begin,
		batch.seq_id + begin,
		batch.logits + begin,
	};
}

bool llm_util::init_batch(llama_model* pModel, llama_context* pCtx, string prompt, llama_batch& out_pBatch)
{
	const int32_t ctx_size = llama_n_ctx(pCtx);
	const bool is_first = llama_kv_self_used_cells(pCtx) == 0;

	// tokenize the prompt
	std::vector<llama_token> prompt_tokens = tokenize(pModel, prompt, is_first);

	// Prepare a batch for the prompt
	llama_batch batch = llama_batch_init(ctx_size, 0, 1);
	int32_t num_tokens = std::min((int32_t)prompt_tokens.size(), ctx_size);

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

bool llm_util::init_embedding_batch(llama_model* pModel, llama_context* pCtx, const std::vector<llama_token>& tokens, llama_batch& out_pBatch)
{
	const int32_t ctx_size = llama_n_ctx(pCtx);

	// Prepare a batch for the prompt
	llama_batch batch = llama_batch_init(ctx_size, 0, 1);
	int32_t num_tokens = std::min((int32_t)tokens.size(), ctx_size);

	// Add tokens to batch
	for (int i = 0; i < num_tokens; ++i) {
		batch.token[i] = tokens[i];
		batch.pos[i] = i;  // Position in sequence
		batch.n_seq_id[i] = 1;  // This token belongs to 1 sequence
		batch.seq_id[i][0] = 0;  // Sequence ID 0
		batch.logits[i] = false;  // Don't need logits for most tokens
	}
	batch.n_tokens = num_tokens;

	out_pBatch = batch;
	return true;
}

std::vector<llama_token> llm_util::tokenize_and_batch(llama_model* pModel, llama_context* pCtx, llama_batch& batch, string content, int32_t pos, bool add_special)
{
	// Add to context batch
	auto tokens = llm_util::tokenize(pModel, content, add_special);
	batch.n_tokens += batch_write(pModel, pCtx, batch, tokens, pos);
	return tokens;
}

int32_t llm_util::batch_write(llama_model* pModel, llama_context* pCtx, llama_batch& batch, const std::vector<llama_token>& tokens, int32_t pos)
{
	// Add to context batch
	int32_t n_tokens = (int32_t)tokens.size();
	for (int32_t i = 0; i < n_tokens; ++i)
	{
		int idx = pos + i;
		batch.token[idx] = tokens[i];
		batch.pos[idx] = idx;
		batch.n_seq_id[idx] = 1;
		batch.seq_id[idx][0] = 0;
		batch.logits[i] = false;
	}
	return n_tokens;
}

std::optional<std::vector<llama_token>> llm_util::tokenize_and_decode(llama_model* pModel, llama_context* pCtx, llama_batch& batch, string content, int32_t pos, bool add_special)
{
	auto tokens = tokenize_and_batch(pModel, pCtx, batch, content, pos, add_special);
	int32_t n_tokens = toI(tokens.size());

	llama_batch batch_view = llm_util::create_batch_view(batch, pos, pos + n_tokens);
	if (batch_view.n_tokens > 0 && llama_decode(pCtx, batch_view) != 0)
		return std::nullopt;
	return tokens;
}

struct Span
{
	MessageType msgType {};
	size_t start;
	size_t end; // exclusive
	size_t length() const { return end - start; }
};

static size_t find_next(const std::string& text, const std::string& substring, size_t start = 0)
{
	char ch = substring[0];
	for (size_t pos = start; pos <= text.size() - substring.size(); ++pos)
	{
		if (text[pos] == ch)
		{
			bool match = true;
			for (size_t n = 1; n < substring.size(); ++n)
			{
				if (text[pos + n] != substring[n])
				{
					match = false;
					break;
				}
			}
			if (match)
				return pos;
		}
	}
	return std::string::npos;
}

static bool in_span(size_t pos, const std::vector<Span>& spans)
{
	if (pos == std::string::npos)
		return false;

	for (auto s : spans)
	{
		if (pos >= s.start && pos < s.end)
			return true;
	}
	return false;
}

static void mark_spans(const std::string s, MessageType msgType, std::string open, std::string close, std::vector<Span>& spans)
{
	if (open.size() > s.size())
		return;

	size_t pos_open = find_next(s, open);
	while (pos_open != std::string::npos)
	{
		if (in_span(pos_open, spans))
		{
			pos_open = find_next(s, open, pos_open + 1);
			continue;
		}

		size_t pos_close = find_next(s, close, pos_open + open.size());
		if (in_span(pos_open, spans))
		{
			pos_close = find_next(s, close, pos_close + 1);
			continue;
		}

		if (pos_close == std::string::npos)
			return;

		spans.push_back(Span { msgType, pos_open, pos_close + close.size() });
		pos_open = find_next(s, open, pos_close + close.size());
	}
}

static void fill_gaps(const std::string s, MessageType msgType, std::vector<Span>& spans)
{
	auto CheckAndAdd = [msgType, &spans](size_t pos, size_t len) {
		if (len != 0)
			spans.push_back(Span { msgType, pos, pos + len });
		return true;
	};

	// Sort spans
	std::sort(spans.begin(), spans.end(), [](Span a, Span b) {
		return a.start < b.start;
	});

	size_t length = s.length();

	if (spans.size() == 0)
	{
		CheckAndAdd(0, length);
		return;
	}

	size_t n = spans.size() - 1;
	CheckAndAdd(spans[spans.size() - 1].end, length - spans[spans.size() - 1].end); // Tail
	CheckAndAdd(0, spans[0].start); // Head

	for (size_t i = 0; i < n; ++i)
	{
		auto& first = spans[i];
		auto& second = spans[i + 1];
		if (first.end < second.start)
			CheckAndAdd(first.end, second.start - first.end);
	}

	// Sort again
	std::sort(spans.begin(), spans.end(), [](Span a, Span b) {
		return a.start < b.start;
	});
}

static void trim_spans(const std::string s, std::vector<Span>& spans)
{
	for (auto& span : spans)
	{
		for (size_t pos = span.end - 1; pos > span.start; --pos)
		{
			if (std::isspace((unsigned char)s[pos]))
				span.end = pos;
			else
				break;
		}
		
		for (size_t pos = span.start; pos < span.end; ++pos)
		{
			span.start = pos;
			if (!std::isspace((unsigned char)s[pos]))
				break;
		}
	}
}

std::string llm_util::process_message(std::string message, std::string identifier, std::vector<Submessage>* out_pSubmessages) noexcept
{
	size_t pos = 0;
	size_t length = message.size();

	string_util::trim_str(message);
	char first = message[0];
	char last = 0;
	if (first == '"' || first == '*')
		last = first;
	else if (first == '[')
		last = ']';
	else if (first == '(')
		last = ')';
	else if (first == '{')
		last = '}';
	if (last != 0)
	{
		size_t pos_last = message.find(last, 1);
		if (pos_last == string::npos)
			message += last;
	}

	std::vector<Span> spans;
	spans.reserve(64);

	mark_spans(message, MessageType::Dialogue, "\"", "\"", spans);
	mark_spans(message, MessageType::Action, "*", "*", spans);
	mark_spans(message, MessageType::Thought, "((", "))", spans);
	mark_spans(message, MessageType::Narration, "[", "]", spans);
	mark_spans(message, MessageType::Direction, "{{", "}}", spans);

	fill_gaps(message, MessageType::Dialogue, spans);
	trim_spans(message, spans);

	std::string result;
	result.reserve(256);
	for (auto& span : spans)
	{
		std::string text = message.substr(span.start, span.end - span.start);
		switch (span.msgType)
		{
		case MessageType::Dialogue:
			if (text[0] == '"')
			{
				text.erase(text.length() - 1, 1);
				text.erase(0, 1);
			}
			break;
		case MessageType::Action:
		case MessageType::Narration:
			text.erase(text.length() - 1, 1);
			text.erase(0, 1);
			break;
		case MessageType::Thought:
		case MessageType::Direction:
			text.erase(text.length() - 2, 2);
			text.erase(0, 2);
			break;
		}
		
		text = string_util::trim(text);
		if (string_util::empty_or_whitespace(text))
			continue;

		if (span.msgType == MessageType::Dialogue) // Minor processing for correctness
		{
			std::wstring uniText = string_util::from_utf8(text);
			if (!std::iswpunct(uniText.front()) && !std::iswupper(uniText.front()))
				uniText[0] = std::toupper(uniText[0]); // Uppercase first letter
			if (!std::iswpunct(uniText.back()))
				uniText.append(L"."); // Require punctuation
			text = string_util::to_utf8(uniText);
		}

		if (result.length() > 0)
			result.append(" ");

		switch (span.msgType)
		{
		case MessageType::Dialogue:
			result.append(std::format("<{0}=\"{1}\">\"{2}\"</{0}>", Constants::DialogueTag, identifier, text));
			break;
		case MessageType::Action:
			result.append(std::format("<{0}=\"{1}\">*{2}*</{0}>", Constants::ActionTag, identifier, text));
			break;
		case MessageType::Thought:
			result.append(std::format("<{0}=\"{1}\">({2})</{0}>", Constants::ThoughtTag, identifier, text));
			break;
		case MessageType::Narration:
			result.append(std::format("<{0}>[{1}]</{0}>", Constants::NarrationTag, text));
			break;
		case MessageType::Direction:
			result.append(std::format("<{0}>{1}</{0}>", Constants::DirectionTag, text));
			break;
		}

		if (out_pSubmessages != nullptr)
		{
			(*out_pSubmessages).push_back(Submessage {
				span.msgType,
				text,
			});
		}
	}

	return result;
}

string llm_util::format_id(string id)
{ 
	if (id[0] == '@')
		id = id.substr(1);
	return string_util::lcase(id);
}

int32_t llm_util::erase_tokens(llama_context* pCtx, llama_batch& batch, int32_t from, int32_t length)
{
	if (from < 0 || from >= Constants::ContextSize || length <= 0  || from + length > Constants::ContextSize)
		return 0;

	llama_kv_self_seq_rm(pCtx, 0, from, from + length);

	for (int32_t i = 0; i < length; ++i)
	{
		int32_t idx = from + i;
		batch.pos[idx] = 0;
		batch.token[idx] = 0;
		batch.n_seq_id[idx] = 0;
		batch.logits[idx] = false;
	}
	batch.n_tokens -= length;
	return length;
}

int32_t llm_util::batch_remove(llama_context* pCtx, llama_batch& batch, int32_t begin, int32_t end)
{
	int32_t n_removed = end - begin;

	// Remove
	llama_kv_self_seq_rm(pCtx, 0, begin, end);
	llama_kv_self_seq_add(pCtx, 0, end, -1, -n_removed);
	llama_kv_self_update(pCtx);

	// Update batch
	int32_t n_batch = batch.n_tokens;
	for (int32_t i = 0; i < n_removed; ++i)
	{
		batch.token[begin + i] = batch.token[end + i];
		batch.n_seq_id[begin + i] = batch.n_seq_id[end + i];
		batch.pos[begin + i] = begin + i;
		batch.seq_id[begin + i][0] = batch.seq_id[end + i][0];
		batch.logits[i] = false;
	}
	batch.n_tokens -= n_removed;
	return n_removed;
}


int32_t llm_util::batch_allocate(llama_context* pCtx, llama_batch& batch, int32_t pos, int32_t length)
{
	// Remove
	llama_kv_self_seq_add(pCtx, 0, pos, -1, length);
	llama_kv_self_update(pCtx);

	int32_t ctx_size = llama_n_ctx(pCtx);

	// Update batch
	int32_t n_batch = batch.n_tokens;
	for (int32_t i = 0; i < n_batch - pos; ++i)
	{
		if (i >= ctx_size)
			continue;

		int32_t idx = n_batch + length - i - 1;
		batch.pos[idx] = idx;
		batch.token[idx] = batch.token[idx - length];
		batch.n_seq_id[idx] = batch.n_seq_id[idx - length];
		batch.seq_id[idx][0] = batch.seq_id[idx - length][0];
		batch.logits[idx] = false;
	}
	batch.n_tokens += length;

	// Clear allocated tokens
	for (int32_t i = 0; i < length; ++i)
	{
		int32_t idx = pos + i;
		batch.pos[idx] = -1;
		batch.token[idx] = 0;
		batch.n_seq_id[idx] = 0;
		batch.logits[idx] = false;
	}

	return length;
}

int32_t llm_util::shift_tokens(llama_context* pCtx, llama_batch& batch, int32_t pos, int32_t len, int32_t shift_amount)
{
	// Shift down
	int32_t ctx_size = llama_n_ctx(pCtx);
	if (len < 0)
		len = ctx_size - pos;

	int32_t src_pos = pos;
	int32_t dest_pos = pos + shift_amount;
	if (src_pos > dest_pos) // Shifting up, write top down
	{
		for (int32_t i = 0; i < shift_amount; ++i)
		{
			int idx = dest_pos + i;
			if (idx < 0 || idx >= ctx_size)
				continue;
			batch.pos[idx] = idx;
			batch.token[idx] = batch.token[src_pos + i];
			batch.n_seq_id[idx] = batch.n_seq_id[src_pos + i];
			batch.seq_id[idx][0] = batch.seq_id[src_pos + i][0];
			batch.logits[idx] = batch.logits[src_pos + i];
		}
	}
	else if (src_pos < dest_pos) // Shifting down, write bottom up
	{
		for (int32_t i = 0; i < shift_amount; ++i)
		{
			int idx = dest_pos + len - i - 1;
			if (idx < 0 || idx >= ctx_size)
				continue;
			batch.pos[idx] = idx;
			batch.token[idx] = batch.token[src_pos + len - i - 1];
			batch.n_seq_id[idx] = batch.n_seq_id[src_pos + len - i - 1];
			batch.seq_id[idx][0] = batch.seq_id[src_pos + len - i - 1][0];
			batch.logits[idx] = batch.logits[src_pos + len - i - 1];
		}		
	}

	// Apply down-shifts
	llama_kv_self_seq_add(pCtx, 0, pos, -1, shift_amount);
	return shift_amount;
}

int32_t llm_util::ctx_remove(llama_context* pCtx, ContextState& ctxState, std::vector<ContextBlock>::iterator itBegin, std::vector<ContextBlock>::iterator itEnd)
{
	int32_t shift_amount = 0;
	for (auto it = itBegin; it != itEnd; ++it)
		shift_amount += (*it).length();
	
	int32_t pos_remove_begin = ctxState.blocks_pos + (*itBegin).offset;
	int32_t pos_remove_end = pos_remove_begin + shift_amount;

	if (llama_kv_self_seq_rm(pCtx, 0, pos_remove_begin, pos_remove_end))
	{
		ctxState.blocks.erase(itBegin, itEnd);
		return shift_amount;
	}

	return 0; // Error
}

int32_t llm_util::ctx_remove_and_shift(llama_context* pCtx, ContextState& ctxState, std::vector<ContextBlock>::iterator itBegin, std::vector<ContextBlock>::iterator itEnd)
{
	// Remove
	llama_pos pos_remove_begin = ctxState.blocks_pos + (*itBegin).offset;
	int32_t shift_amount = ctx_remove(pCtx, ctxState, itBegin, itEnd);
	if (shift_amount == 0)
		return 0;

	llama_pos pos_remove_end = pos_remove_begin + shift_amount;

	// Shift
	llama_kv_self_seq_add(pCtx, 0, pos_remove_end, -1, -shift_amount);

	// Update batch
	auto& batch = ctxState.batch;
	int32_t n_batch = batch.n_tokens;
	for (int32_t i = 0; i < n_batch - pos_remove_end; ++i)
	{
		batch.token[pos_remove_begin + i] = batch.token[pos_remove_end + i];
		batch.n_seq_id[pos_remove_begin + i] = batch.n_seq_id[pos_remove_end + i];
		batch.pos[pos_remove_begin + i] = pos_remove_begin + i;
		batch.seq_id[pos_remove_begin + i][0] = batch.seq_id[pos_remove_end + i][0];
		batch.logits[i] = false;
	}
	batch.n_tokens -= shift_amount;
	return (int32_t)-shift_amount;
}