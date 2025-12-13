#include "llm/LLMUtility.h"
#include "util/StringUtility.h"
#include "util/FileUtility.h"
#include "util/Common.h"
#include "Constants.h"
#include <format>
#include <cwctype>
#include <cassert>
#include <set>

static std::vector<string> const opening_tags {
	std::format("<{0}=\"", Constants::Chat::DialogueTag),
	std::format("<{0}=\"", Constants::Chat::ActionTag),
	std::format("<{0}=\"", Constants::Chat::ThoughtTag),
	std::format("<{0}>", Constants::Chat::NarrationTag),
	std::format("<{0}>", Constants::Chat::DirectionTag),
};

static std::vector<string> const closing_tags {
	std::format("</{0}>", Constants::Chat::DialogueTag),
	std::format("</{0}>", Constants::Chat::ActionTag),
	std::format("</{0}>", Constants::Chat::ThoughtTag),
	std::format("</{0}>", Constants::Chat::NarrationTag),
	std::format("</{0}>", Constants::Chat::DirectionTag),
};

std::string llm_util::stringFromToken(VocabPtr pVocab, llama_token token)
{
	// convert the token to a string, print it and add it to the response
	char buf[256];
	int n = llama_token_to_piece(pVocab, token, buf, sizeof(buf), 0, false);
	if (n < 0)
		return "";

	return std::string(buf, n);
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
		{ std::format("<{0}", Constants::Chat::DialogueTag),		MessageType::Dialogue},
		{ std::format("<{0}", Constants::Chat::ActionTag),		MessageType::Action},
		{ std::format("<{0}", Constants::Chat::ThoughtTag),		MessageType::Thought},
		{ std::format("<{0}", Constants::Chat::NarrationTag),		MessageType::Narration},
		{ std::format("<{0}", Constants::Chat::DirectionTag),		MessageType::Direction},
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
		text = std::format("<{0}=\"{{{{char}}}}\">{1}</{0}>", Constants::Chat::DialogueTag, text);
		break;
	case MessageType::Dialogue:
		text.append(std::format("\"</{0}>", Constants::Chat::DialogueTag));
		break;
	case MessageType::Action:
		text.append(std::format("*</{0}>", Constants::Chat::ActionTag));
		break;
	case MessageType::Thought:
		text.append(std::format("))</{0}>", Constants::Chat::ThoughtTag));
		break;
	case MessageType::Narration:
		text.append(std::format("]</{0}>", Constants::Chat::NarrationTag));
		break;
	case MessageType::Direction:
		text.append(std::format("</{0}>", Constants::Chat::DirectionTag));
		break;
	case MessageType::SystemMessage:
	default:
		break;
	}
	return text;
}

void llm_util::process(string& partial, string str_token, bool* bWait, bool* bHalt, string& stop_word)
{
	if (string_util::validate_utf8(partial) < partial.size()) // Incomplete utf-8 string
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

std::vector<llama_token> llm_util::tokenize(VocabPtr pVocab, string prompt, bool add_special)
{
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

llama_batch llm_util::init_batch(int32_t ctx_size, int32_t n_seq_max)
{
	// Prepare a batch for the prompt
	llama_batch batch = llama_batch_init(ctx_size, 0, n_seq_max);
	batch.n_tokens = 0;

	for (size_t i = 0; i < ctx_size; ++i)
	{
		batch.pos[i] = (int32_t)i;
		batch.token[i] = 0;
		batch.n_seq_id[i] = 0;
		for (size_t itSeq = 1; itSeq < n_seq_max; ++itSeq)
			batch.seq_id[i][itSeq] = -1;
		batch.logits[i] = false;
	}
	return batch;
}

void llm_util::free_batch(llama_batch& batch)
{
	llama_batch_free(batch);

	batch.pos = nullptr;
	batch.token = nullptr;
	batch.embd = nullptr;
	batch.logits = nullptr;
	batch.n_seq_id = nullptr;
	batch.seq_id = nullptr;
	batch.n_tokens = 0;
}

llama_batch llm_util::create_batch(std::span<llama_token> tokens, std::span<llama_seq_id> seqs, int32_t n_seq_max, int32_t position)
{
	// Prepare a batch for the prompt
	llama_batch batch = llama_batch_init(toI(tokens.size()), 0, n_seq_max);
	batch.n_tokens = toI(tokens.size());
	batch.embd = nullptr;

	size_t i = 0;
	for (auto token : tokens)
	{
		batch.pos[i] = (int32_t)i + position;
		batch.token[i] = token;
		batch.n_seq_id[i] = toI(seqs.size());
		batch.logits[i] = 0;
		std::copy(seqs.begin(), seqs.end(), batch.seq_id[i]);
		++i;
	}
	return batch;
}

llama_batch llm_util::create_batch_view(const llama_batch& batch, int32_t position, int32_t length)
{
	return llama_batch {
		length,
		batch.token + position,
		nullptr,
		batch.pos + position,
		batch.n_seq_id + position,
		batch.seq_id + position,
		batch.logits + position,
	};
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
		batch.pos[i] = i;		// Position in sequence
		batch.n_seq_id[i] = 1;	// This token belongs to 1 sequence
		batch.seq_id[i][0] = 0;	// Sequence ID 0 //! @seq
		batch.logits[i] = true;
	}
	batch.n_tokens = num_tokens;

	out_pBatch = batch;
	return true;
}

std::optional<std::vector<llama_token>> llm_util::tokenize_and_decode(Context& context, string content, SequenceId seq_id, int32_t pos, bool add_special)
{
	auto tokens = tokenize_and_batch(context, content, seq_id, pos, add_special);
	int32_t n_tokens = toI(tokens.size());

	llama_batch batch_view = context.GetCache().GetBatchView(pos, n_tokens);
	if (batch_view.n_tokens > 0 && llama_decode(context.GetCtxPtr(), batch_view) != 0)
		return std::nullopt;
	return tokens;
}

std::vector<llama_token> llm_util::tokenize_and_batch(Context& context, string content, SequenceId seq_id, int32_t pos, bool add_special)
{
	// Add to context batch
	auto tokens = llm_util::tokenize(context.GetVocabPtr(), content, add_special);
	context.GetCache().BatchWrite(tokens, seq_id, pos);
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
			result.append(std::format("<{0}=\"{1}\">\"{2}\"</{0}>", Constants::Chat::DialogueTag, identifier, text));
			break;
		case MessageType::Action:
			result.append(std::format("<{0}=\"{1}\">*{2}*</{0}>", Constants::Chat::ActionTag, identifier, text));
			break;
		case MessageType::Thought:
			result.append(std::format("<{0}=\"{1}\">({2})</{0}>", Constants::Chat::ThoughtTag, identifier, text));
			break;
		case MessageType::Narration:
			result.append(std::format("<{0}>[{1}]</{0}>", Constants::Chat::NarrationTag, text));
			break;
		case MessageType::Direction:
			result.append(std::format("<{0}>{1}</{0}>", Constants::Chat::DirectionTag, text));
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

bool llm_util::dump_batch_text(const Context& context, int32_t seq_index, string filename)
{
	auto [batch_ref, batch_n] = context.GetCache().GetBatch();
	auto& batch = batch_ref.get();
	auto pVocab = context.GetVocabPtr();

	auto fnTokenStr = [pVocab](llama_token token, bool quote) -> string {
		if (token <= 0)
			return "<UNK>";
		else if (token == llama_vocab_bos(pVocab))
			return "<BOS>";
		else if (token == llama_vocab_eos(pVocab))
			return "<EOS>";
		else if (token == llama_vocab_eot(pVocab))
			return "<EOT>";
		else if (token == llama_vocab_sep(pVocab))
			return "<SEP>";
		else if (token == llama_vocab_pad(pVocab))
			return "<PAD>";
		else if (token == llama_vocab_nl(pVocab))
			return "\r\n";
		else
		{
			char buf[256];
			int n = llama_token_to_piece(pVocab, token, buf, sizeof(buf), 0, true);
			if (n < 0)
				return "<UNK>";
			else
				return quote ? "\"" + string(buf, n) + "\"" : string(buf, n);
		}
	};

	if (batch.token == nullptr)
		return false;

	// Detokenize the batched tokens
	string result;
	result.reserve(65536);
	for (int32_t i = 0; i < batch_n; ++i)
	{
		if (seq_index >= 0)
		{
			bool bFound = false;
			for (int32_t itSeq = 0; itSeq < batch.n_seq_id[i]; ++itSeq)
				bFound |= (batch.seq_id[i][itSeq] == seq_index);
			if (!bFound)
				continue;
		}

		result.append(fnTokenStr(batch.token[i], false));
	}

	result.append(std::format("[pos:{0}/{1}]\r\n", context.cursor_pos, batch_n));

	// Cached blocks
	SequenceId seq_id = sequence_from_index(seq_index);
	auto& blocks = context.GetBlocks();
	for (auto& block : blocks)
	{
		if (block.is_cached())
			continue;
		if ((bool)(block.sequenceId & seq_id) == false)
			continue;

		result.append("[");
		if (!block.tokens.empty())
		{
			for (int32_t i = 0; i < block.length(); ++i)
				result.append(fnTokenStr(block.tokens[i], false));
		}
		else
		{
			result.append(block.content);
		}
		result.append("]\r\n");
	}

	return file_util::WriteTextFile(filename, result, false) == FileError::NoError;
}

bool llm_util::dump_batch_tokens(const Context& context, int32_t seq_id, string filename)
{
	auto [batch_ref, batch_n] = context.GetCache().GetBatch();

	return dump_batch_tokens(batch_ref, batch_n, seq_id, context.GetVocabPtr(), filename);
}

bool llm_util::dump_batch_tokens(const llama_batch& batch, int32_t num_tokens, int32_t seq_index, VocabPtr pVocab, string filename)
{
	auto fnTokenStr = [pVocab](llama_token token) -> string {
		if (token == llama_vocab_bos(pVocab))
			return "<BOS>";
		else if (token == llama_vocab_eos(pVocab))
			return "<EOS>";
		else if (token == llama_vocab_eot(pVocab))
			return "<EOT>";
		else if (token == llama_vocab_sep(pVocab))
			return "<SEP>";
		else if (token == llama_vocab_pad(pVocab))
			return "<PAD>";
		else if (token == llama_vocab_nl(pVocab))
			return "<NL>"; //"\r\n";
		else
		{
			if (token < 0 || token > 32000)
				return "<UNK>"; // Error

			char buf[256];
			int n = llama_token_to_piece(pVocab, token, buf, sizeof(buf), 0, true);
			if (n < 0)
				return "<UNK>";
			else
				return string(buf, n);
		}
	};

	if (batch.token == nullptr || num_tokens <= 0)
		return false;

	// Detokenize the batched tokens
	string result;
	result.reserve(65536);
	for (int32_t i = 0; i < num_tokens; ++i)
	{
		if (seq_index >= 0)
		{
			bool bFound = false;
			for (int32_t itSeq = 0; itSeq < batch.n_seq_id[i]; ++itSeq)
				bFound |= (batch.seq_id[i][itSeq] == seq_index);
			if (!bFound)
				continue;
		}

		int32_t seq_id = 0;
		int32_t n_seq_id = batch.n_seq_id[i];
		for (int32_t it_seq = 0; it_seq != n_seq_id; ++it_seq)
		{
			int32_t seq = batch.seq_id[i][it_seq];
			if (seq >= 0)
				seq_id |= 1 << seq;
		}

		result.append(std::format("{0:<8} {1:<8} {2:<4x} {4:<8} \"{5}\"\r\n",
			batch.pos[i],
			batch.token[i],
			seq_id,
			n_seq_id,
			(int32_t)batch.logits[i],
			fnTokenStr(batch.token[i]))
		);
	}

	return file_util::WriteTextFile(filename, result, false) == FileError::NoError;
}

bool llm_util::dump_kv_cache(const Context& context, int32_t seq_id, string filename)
{
	auto cache_view = llama_kv_cache_view_init(context.GetCtxPtr(), context.GetCache().n_seq_max());
	llama_kv_cache_view_update(context.GetCtxPtr(), &cache_view);

	int32_t n_max_seq = cache_view.n_seq_max;
	std::vector<int32_t> cells;
	cells.resize(cache_view.n_cells);
	std::vector<int32_t> batched;
	batched.resize(cache_view.n_cells);

	for (int32_t it_cell = 0; it_cell < cache_view.n_cells; ++it_cell)
	{
		auto& cell = cache_view.cells[it_cell];
		if (cell.pos < 0)
			continue;

		llama_seq_id* cell_seqs = &cache_view.cells_sequences[it_cell * n_max_seq];
		for (llama_seq_id* pSeq = cell_seqs; pSeq < cell_seqs + ptrdiff_t(n_max_seq); ++pSeq)
		{
			if (*pSeq == seq_id)
			{
				++cells[cell.pos];
				break;
			}
		}
	}
	
	auto const [batch_ref, batch_n] = context.GetCache().GetBatch();
	auto& batch = batch_ref.get();
	for (int32_t idx = 0; idx < cache_view.n_cells && idx < batch_n; ++idx)
	{
		if (batch.pos[idx] < 0)
			continue;

		int32_t tok_pos = batch.pos[idx];
		int32_t tok_n_seqs = batch.n_seq_id[idx];
		int32_t* tok_seqs = batch.seq_id[idx];
		for (int32_t* pSeq = tok_seqs; pSeq < tok_seqs + ptrdiff_t(tok_n_seqs); ++pSeq)
		{
			if (*pSeq == seq_id)
			{
				++batched[tok_pos];
				break;
			}
		}
	}

	string result;
	result.reserve(32384);
	for (int32_t i = 0; i < (int32_t)cells.size(); ++i)
	{
		if (i % 64 == 0)
		{
			if (i > 0)
				result.append("\r\n");
			result.append(std::format("[{:<5}] ", i));
		}
		else if (i % 8 == 0 && i > 0)
			result.append(" ");

		if (cells[i] == 0)
		{
			if (batched[i] == 0)
				result.append(context.cursor_pos == i ? "_" : ".");
			else
				result.append(context.cursor_pos == i ? ">" : "o");
		}
		else if (cells[i] == 1)
			result.append(context.cursor_pos == i ? "0" : "O");
		else 
			result.append("D");
	}

	llama_kv_cache_view_free(&cache_view);
	
	return file_util::WriteTextFile(filename, result, false) == FileError::NoError;
}

bool llm_util::dump_kv_cache_cells(const Context& contextState, string filename)
{
	return dump_kv_cache_cells(contextState.GetCtxPtr(), contextState.GetNumSequences(), filename);
}

bool llm_util::dump_kv_cache_cells(llama_context* pCtx, int32_t num_sequences, string filename)
{
	auto cache_view = llama_kv_cache_view_init(pCtx, num_sequences);

	llama_kv_cache_view_update(pCtx, &cache_view);

	int32_t n_max_seq = cache_view.n_seq_max;
	std::vector<int32_t> cells;
	cells.resize(cache_view.n_cells);

	for (int32_t it_cell = 0; it_cell < cache_view.n_cells; ++it_cell)
	{
		auto& cell = cache_view.cells[it_cell];
		llama_seq_id* cell_seqs = &cache_view.cells_sequences[it_cell * n_max_seq];
		
		cells[it_cell] = 0;

		for (int32_t it_seq = 0; it_seq != n_max_seq; ++it_seq)
		{
			int32_t seq = static_cast<int32_t>(*(cell_seqs + it_seq));
			if (seq >= 0)
				cells[it_cell] |= 1 << seq;
		}
	}
	
	string result;
	result.reserve(32384);
	for (int32_t i = 0; i < (int32_t)cells.size(); ++i)
	{
		if (i % 64 == 0)
		{
			if (i > 0)
				result.append("\r\n");
			result.append(std::format("[{:<5}] ", i));
		}
		else if (i % 8 == 0 && i > 0)
			result.append(" ");

		if (cells[i] == 0)
			result.append(".");
		else if (cells[i] <= 0xf)
			result.append(std::format("{:x}", cells[i]));
		else
			result.append("X");
	}

	llama_kv_cache_view_free(&cache_view);
	
	return file_util::WriteTextFile(filename, result, false) == FileError::NoError;
}

void llm_util::erase_bottom(llama_context* pCtx, int32_t n_max_seq, int32_t pos)
{
	for (int32_t seq_id = 0; seq_id < n_max_seq; ++seq_id)
		llama_kv_self_seq_rm(pCtx, seq_id, pos, -1);
}

SequenceIndices llm_util::get_sequence_indices(Sequence seq, int32_t n_seq_max) noexcept
{
	return get_sequence_indices({ seq }, n_seq_max);
}

SequenceIndices llm_util::get_sequence_indices(SequenceId seq, int32_t n_seq_max) noexcept
{
	SequenceIndices seqIds;
	seqIds.reserve(n_seq_max);

	for (size_t i = 0; i < AllSequenceIDs.size() && i < n_seq_max; ++i)
	{
		if (seq.IsSet(AllSequenceIDs[i]))
			seqIds.push_back(toI(i));
	}
	return seqIds;
}

SequenceId llm_util::sequence_from_index(int32_t seq_idx) noexcept
{
    if (seq_idx < 0 || seq_idx >= AllSequenceIDs.size())
        return SequenceId::None;
	return { AllSequenceIDs[seq_idx] };
}