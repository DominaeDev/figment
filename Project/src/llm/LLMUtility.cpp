#include <pch.h>
#include "llm/LLMUtility.h"
#include "llm/LlamaApi.h"
#include "llm/ModelState.h"
#include "io/FileUtility.h"
#include <format>
#include <cwctype>
#include <cassert>
#include <set>

using namespace fig::io;
using namespace fig::chat;

namespace fig::llm
{
	static std::vector<fig::string> const opening_tags {
		std::format("<{0}=\"", fig::Constants::Chat::DialogueTag),
		std::format("<{0}=\"", fig::Constants::Chat::ActionTag),
		std::format("<{0}=\"", fig::Constants::Chat::ThoughtTag),
		std::format("<{0}>",   fig::Constants::Chat::NarrationTag),
		std::format("<{0}>",   fig::Constants::Chat::DirectionTag),
	};	

	static std::vector<fig::string> const closing_tags {
		std::format("</{0}>", fig::Constants::Chat::DialogueTag),
		std::format("</{0}>", fig::Constants::Chat::ActionTag),
		std::format("</{0}>", fig::Constants::Chat::ThoughtTag),
		std::format("</{0}>", fig::Constants::Chat::NarrationTag),
		std::format("</{0}>", fig::Constants::Chat::DirectionTag),
	};

	size_t string_find_partial_stop(const fig::string& str, const fig::string& stop)
	{
		if (!str.empty() && !stop.empty())
		{
			const char text_last_char = str.back();
			for (int64_t char_index = stop.size() - 1; char_index >= 0; char_index--)
			{
				if (stop[char_index] == text_last_char)
				{
					const auto current_partial = stop.substr(0, char_index + 1);
					if (ends_with(str, current_partial))
					{
						return str.size() - char_index - 1;
					}
				}
			}
		}

		return fig::npos;
	}

	size_t find_one_of(const fig::string& text, const std::vector<fig::string>& words)
	{
		size_t stop_pos = fig::npos;

		for (const fig::string& word : words)
		{
			size_t pos = text.find(word);
			if (pos != fig::npos && (stop_pos == fig::npos || pos < stop_pos))
				stop_pos = pos;
		}

		return stop_pos;
	}

	size_t find_stopping_strings(const fig::string& text, const std::vector<fig::string>& stop_words, const size_t last_token_size, bool is_full_stop)
	{
		size_t stop_pos = fig::npos;

		for (const fig::string& word : stop_words)
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

			if (pos != fig::npos && (stop_pos == fig::npos || pos < stop_pos))
			{
				stop_pos = pos;
			}
		}

		return stop_pos;
	}

	void get_tag_and_name(const fig::string& text, fig::string& tag, fig::string& name)
	{
		size_t pos_equals = text.find('=', 1);
		if (pos_equals == fig::npos)
		{
			tag = trim(text.substr(1, text.length() - 2));
			name = "";
			return;
		}

		tag = trim(text.substr(1, pos_equals - 1));
		name = trim(text.substr(pos_equals + 1, text.length() - pos_equals - 2));
		replace_all_inplace(name, "\"", "");
	}

	std::pair<MessageType, bool> detect_message_type(fig::string text) noexcept
	{
		auto patterns = std::vector<std::tuple<fig::string, MessageType>> {
			{ std::format("<{0}", Constants::Chat::DialogueTag),		MessageType::Dialogue},
			{ std::format("<{0}", Constants::Chat::ActionTag),		MessageType::Action},
			{ std::format("<{0}", Constants::Chat::ThoughtTag),		MessageType::Thought},
			{ std::format("<{0}", Constants::Chat::NarrationTag),		MessageType::Narration},
			{ std::format("<{0}", Constants::Chat::DirectionTag),		MessageType::Direction},
		};

		size_t last_pos = fig::npos;
		MessageType msgType = MessageType::Undefined;
		for (int i = 0; i < patterns.size(); ++i)
		{
			size_t pos = text.rfind(std::get<0>(patterns[i]), fig::npos);
			if (pos != fig::npos && (last_pos == fig::npos || pos > last_pos))
			{
				last_pos = pos;
				msgType = std::get<1>(patterns[i]);
			}
		}

		bool bComplete = false;
		for (auto& tag : closing_tags)
		{
			size_t pos_end = text.rfind(tag, fig::npos);
			if (pos_end != fig::npos && pos_end > last_pos)
			{
				bComplete = true;
				break;
			}
		}

		return std::make_pair(msgType, bComplete);
	}

	fig::string& sanitize_response(fig::string& text)
	{
		size_t pos_last = text.find_last_of('<');
		if (pos_last == fig::npos)
			return text;
		size_t pos_close = text.find_last_of('>');
		if (pos_close == fig::npos || pos_close < pos_last)
			text = text.substr(0, pos_last);
		return text;
	}

	fig::string& complete_message(fig::string& text)
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

	void process(fig::string& partial, fig::string str_token, bool* bWait, bool* bHalt, fig::string& stop_word)
	{
		if (validate_utf8(partial) < partial.size()) // Incomplete utf-8 string
		{
			*bWait = true;
			*bHalt = false;
			return;
		}

		static std::vector<fig::string> stop_words {
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

		static std::vector<fig::string> formatting_tags;
		if (formatting_tags.empty())
		{
			formatting_tags.insert(formatting_tags.end(), opening_tags.begin(), opening_tags.end());
			formatting_tags.insert(formatting_tags.end(), closing_tags.begin(), closing_tags.end());
		}

		// Look for stop word - and halt
		size_t stop_pos = find_stopping_strings(partial, stop_words, str_token.size(), true);
		if (stop_pos != fig::npos)
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
		if (stop_pos != fig::npos)
		{
			*bHalt = false;
			*bWait = true;
			return;
		}

		// Look for formatting tags
		size_t fmt_pos = find_one_of(partial, opening_tags);
		if (fmt_pos != fig::npos)
		{
			// Await end of tag '>', or beginning of a new tag '<' (indicating garbage from the model)
			if (partial.find_first_of("<>", fmt_pos + 1, 2) == fig::npos)
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
			if (fmt_pos != fig::npos)
			{
				*bHalt = false;
				*bWait = true;
				return;
			}
		}

		*bHalt = false;
		*bWait = false;
	}
	/*
	std::optional<std::vector<Token>> tokenize_and_decode(LLMContext& context, fig::string content, SequenceSlots seq_id, int32_t pos, bool add_special)
	{
		auto tokens = tokenize_and_batch(context, content, seq_id, pos, add_special);
		int32_t n_tokens = toI(tokens.size());

		llama_batch batch_view = context.GetCache().GetView(pos, n_tokens);
		if (batch_view.n_tokens > 0 && llama_decode(context.GetCtxPtr(), batch_view) != 0)
			return std::nullopt;
		return tokens;
	}

	std::vector<Token> tokenize_and_batch(LLMContext& context, fig::string content, SequenceSlots seq_id, int32_t pos, bool add_special)
	{
		// Add to context batch
		auto tokens = llama::tokenize(context.GetVocabPtr(), content, add_special);
		context.GetCache().BatchWrite(tokens, seq_id, pos);
		return tokens;
	}*/

	struct Span
	{
		MessageType msgType {};
		size_t start;
		size_t end; // exclusive
		size_t length() const { return end - start; }
	};

	static size_t find_next(const fig::string& text, const fig::string& substring, size_t start = 0)
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
		return fig::npos;
	}

	static bool in_span(size_t pos, const std::vector<Span>& spans)
	{
		if (pos == fig::npos)
			return false;

		for (auto const& s : spans)
		{
			if (pos >= s.start && pos < s.end)
				return true;
		}
		return false;
	}

	static void mark_spans(const fig::string s, MessageType msgType, fig::string open, fig::string close, std::vector<Span>& spans)
	{
		if (open.size() > s.size())
			return;

		size_t pos_open = find_next(s, open);
		while (pos_open != fig::npos)
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

			if (pos_close == fig::npos)
				return;

			spans.push_back(Span { msgType, pos_open, pos_close + close.size() });
			pos_open = find_next(s, open, pos_close + close.size());
		}
	}

	static void fill_gaps(const fig::string s, MessageType msgType, std::vector<Span>& spans)
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

	static void trim_spans(const fig::string s, std::vector<Span>& spans)
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

	fig::string process_message(fig::string message, fig::string identifier, std::vector<Submessage>* out_pSubmessages) noexcept
	{
		size_t pos = 0;
		size_t length = message.size();

		trim_inplace(message);
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
			if (pos_last == fig::npos)
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

		fig::string result;
		result.reserve(256);
		for (auto& span : spans)
		{
			fig::string text = message.substr(span.start, span.end - span.start);
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

			text = trim(text);
			if (empty_or_whitespace(text))
				continue;

			if (span.msgType == MessageType::Dialogue) // Minor processing for correctness
			{
				fig::wstring uniText = from_utf8(text);
				if (!std::iswpunct(uniText.front()) && !std::iswupper(uniText.front()))
					uniText[0] = std::toupper(uniText[0]); // Uppercase first letter
				if (!std::iswpunct(uniText.back()))
					uniText.append(L"."); // Require punctuation
				text = to_utf8(uniText);
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

	Sequences get_sequence_indices(SequenceSlot seq, int32_t n_seq_max) noexcept
	{
		return get_sequence_indices({ seq }, n_seq_max);
	}

	Sequences get_sequence_indices(SequenceSlots seq, int32_t n_seq_max) noexcept
	{
		Sequences seq_ids;
		seq_ids.reserve(n_seq_max);

		for (size_t i = 0; i < fig::llm::AllSequenceSlots.size() && i < n_seq_max; ++i)
		{
			if (seq.IsSet(fig::llm::AllSequenceSlots[i]))
				seq_ids.push_back(toI(i));
		}
		if (seq_ids.empty())
			seq_ids.push_back(fig::llm::InvalidSequence); // None
		return seq_ids;
	}

	SequenceSlots get_sequence_from_index(int32_t seq_idx) noexcept
	{
		if (seq_idx < 0 || seq_idx >= AllSequenceSlots.size())
			return SequenceSlots::None;
		return { AllSequenceSlots[seq_idx] };
	}

	SequenceSlot get_sequence_slot_from_index(Sequence seq_idx) noexcept
	{
		if (seq_idx < 0 || seq_idx >= AllSequenceSlots.size())
			return SequenceSlot::None;
		return AllSequenceSlots[seq_idx];
	}

	fig::string format_id(fig::string id)
	{
		if (id[0] == '@')
			id = id.substr(1);
		return lcase(id);
	}

	bool dump_batch_text(const LLMContext& context, int32_t seq_index, const fig::path& filename)
	{
		auto [batch_ref, batch_n] = context.GetCache().GetBatch();
		auto& batch = batch_ref.get();
		auto pVocab = context.GetVocabPtr();

		auto fnTokenStr = [pVocab](Token token, bool quote) -> fig::string {
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
					return quote ? "\"" + fig::string(buf, n) + "\"" : fig::string(buf, n);
			}
		};

		if (batch.token == nullptr)
			return false;

		// Detokenize the batched tokens
		fig::string result;
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

		result.append(std::format("[pos:{0}/{1}/{2}]\r\n", context.token_pos, context.cursor_pos, batch_n));

		// Cached blocks
		SequenceSlots seq_id = get_sequence_from_index(seq_index);
		auto& blocks = context.GetBlocks();
		for (auto& block : blocks)
		{
			if (block.is_cached())
				continue;
			if ((bool)(block.sequenceSlots & seq_id) == false)
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

		return WriteTextFile(filename, result, false) == FileError::NoError;
	}

	bool dump_batch_tokens(const LLMContext& context, int32_t seq_id, const fig::path& filename)
	{
		auto [batch_ref, batch_n] = context.GetCache().GetBatch();

		return dump_batch_tokens(batch_ref, batch_n, seq_id, context.GetVocabPtr(), filename);
	}

	bool dump_batch_tokens(const llama_batch& batch, int32_t num_tokens, int32_t seq_index, VocabPtr pVocab, const fig::path& filename)
	{
		auto fnTokenStr = [pVocab](Token token) -> fig::string {
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
					return fig::string(buf, n);
			}
		};

		if (batch.token == nullptr || num_tokens <= 0)
			return false;

		// Detokenize the batched tokens
		fig::string result;
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

			result.append(std::format("{0:<8}{1:<8} {2:<8} {3:<4x} {4:<8} {5} \"{6}\"\r\n",
				i,
				batch.pos[i],
				batch.token[i],
				seq_id,
				n_seq_id,
				(int32_t)batch.logits[i],
				fnTokenStr(batch.token[i]))
			);
		}

		return WriteTextFile(filename, result, false) == FileError::NoError;
	}

	bool dump_kv_cache(const LLMContext& context, int32_t seq_id, const fig::path& filename)
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
					assert(cell.pos >= 0 && cell.pos < cells.size());
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

		fig::string result;
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
					result.append(context.token_pos == i ? "_" : ".");
				else
					result.append(context.token_pos == i ? ">" : "o");
			}
			else if (cells[i] == 1)
				result.append(context.token_pos == i ? "0" : "O");
			else
			{
				result.append("D");
			}
		}

		llama_kv_cache_view_free(&cache_view);

		return WriteTextFile(filename, result, false) == FileError::NoError;
	}

	bool dump_kv_cache_cells(const LLMContext& contextState, const fig::path& filename)
	{
		return dump_kv_cache_cells(contextState.GetCtxPtr(), contextState.GetNumSequences(), filename);
	}

	bool dump_kv_cache_cells(ContextPtr pCtx, int32_t num_sequences, const fig::path& filename)
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

		fig::string result;
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

		return WriteTextFile(filename, result, false) == FileError::NoError;
	}

	bool validate_kv_cache(const fig::llm::LLMContext& context, fig::llm::Sequence sequence, int32_t turn)
	{
#if !_DEBUG
		return true;
#else
		auto pCtx = context.GetCtxPtr();
		auto [batch_ref, batch_n] = context.GetCache().GetBatch();
		auto& batch = batch_ref.get();
		auto& blocks = context.GetBlocks();
		int32_t ctx_size = context.GetModel().ctx_size;
		int32_t n_max_seq = context.GetModel().max_sequences;
		auto filter_seq = get_sequence_slot_from_index(sequence);

		// Check cache positions
		int32_t expected_cache_pos = 0;
		std::vector<ContextBlock> sorted_blocks(std::begin(blocks), std::end(blocks));
		std::sort(std::begin(sorted_blocks), std::end(sorted_blocks), [](const ContextBlock& a, const ContextBlock& b) { return a.cache_position < b.cache_position; });
		for (size_t i = 0; i < sorted_blocks.size(); ++i)
		{
			auto& block = sorted_blocks[i];
			if (not block.is_cached())
				continue;

			if (block.cache_position != expected_cache_pos)
			{
				LogLn(std::format(">> Validation failed on turn {0}: Block at index {1} has an unexpected cache position.", turn, i));
				return false;
			}
			expected_cache_pos += block.length();
		}

		// Check attention positions
		int32_t expected_attn_pos = 0;
		for (size_t i = 0; i < blocks.size(); ++i)
		{
			auto& block = blocks[i];
			if (not block.is_cached())
				continue;

			if (not block.sequenceSlots.IsSet(filter_seq))
				continue;

			// Attention position
			if (block.attn_position > expected_attn_pos)
			{
				LogLn(std::format(">> Validation failed on turn {0}: Block at index {1} has an unexpected attention position.", turn, i));
				return false;
			}
			expected_attn_pos = std::max(expected_attn_pos, block.attn_position + block.length());
		}

		// Check tokens against batch
		for (size_t i = 0; i < blocks.size(); ++i)
		{
			auto& block = blocks[i];
			if (not block.is_cached())
				continue;

			if (not block.sequenceSlots.IsSet(filter_seq))
				continue;

			auto seq_ids = block.get_sequence_ids(context.GetNumSequences());
			for (size_t idx = 0; idx < block.tokens.size(); ++idx)
			{
				size_t pos = block.cache_position + idx;
				if (batch.token[pos] != block.tokens[idx])
				{
					LogLn(std::format(">> Validation failed on turn {0}: Token mismatch at position {1} in block at index {2}.", turn, pos, i));
					return false;
				}

				for (size_t itSeq = 0; itSeq < batch.n_seq_id[pos] && itSeq < n_max_seq; ++itSeq)
				{
					Sequence seq = batch.seq_id[pos][itSeq];
					if (seq == -1)
					{
						LogLn(std::format(">> Validation failed on turn {0}: Invalid sequence id for token at position {1} in block at index {2}.", turn, pos, i));
						return false;
					}

					if (std::find(seq_ids.begin(), seq_ids.end(), seq) == seq_ids.end())
					{
						LogLn(std::format(">> Validation failed on turn {0}: Sequence id mismatch (expected {1}) for token at position {2} in block at index {3}.", turn, seq, pos, i));
						return false;
					}
				}
			}
		}

		// Check kv-cache allocations
		auto cache_view = llama_kv_cache_view_init(pCtx, n_max_seq);
		llama_kv_cache_view_update(pCtx, &cache_view);
		std::vector<char> tokens; // by pos
		tokens.resize(cache_view.n_cells, 0);
		
		for (int32_t it_cell = 0; it_cell < cache_view.n_cells; ++it_cell)
		{
			auto& cell = cache_view.cells[it_cell];
			llama_seq_id* cell_seqs = &cache_view.cells_sequences[it_cell * n_max_seq];
			if (cell.pos < 0)
				continue;

			for (int32_t it_seq = 0; it_seq != n_max_seq; ++it_seq)
			{
				int32_t seq = static_cast<int32_t>(*(cell_seqs + it_seq));
				if (seq == sequence)
				{
					if (cell.pos >= ctx_size)
					{
						LogLn(std::format(">> Validation failed on turn {0}: Token out of bounds ({1}).", turn, cell.pos));
						return false;
					}

					if (tokens[cell.pos] != 0)
					{
						LogLn(std::format(">> Validation failed on turn {0}: Overlapping tokens at position {1}.", turn, cell.pos));
						return false;
					}

					tokens[cell.pos] = 1;
				}
			}
		}
		llama_kv_cache_view_free(&cache_view);

		for (size_t i = 0; i < blocks.size(); ++i)
		{
			auto& block = blocks[i];
			if (!block.is_cached())
				continue;

			if (block.attn_position < 0 || block.cache_position < 0 || block.attn_position >= ctx_size || block.cache_position >= ctx_size)
			{
				LogLn(std::format(">> Validation failed on turn {0}: Block at index {1} has invalid position.", turn, i));
				return false;
			}

			for (int32_t pos = block.attn_position; pos < block.attn_position + block.length(); ++pos)
			{
				if (tokens[pos] == 0)
				{
					LogLn(std::format(">> Validation failed on turn {0}: Token position mismatch in block at index {1}.", turn, i));
					return false;
				}
			}
		}
		return true;
#endif
	}

	void embd_normalize(const std::vector<float>& inp, std::vector<float>& out, int n, int embd_norm)
	{
		embd_normalize(inp.data(), out.data(), n, embd_norm);
	}

	void embd_normalize(const float* inp, float* out, int n, int embd_norm)
	{
		double sum = 0.0;

		switch (embd_norm)
		{
		case -1: // no normalisation
			sum = 1.0;
			break;
		case 0: // max absolute
			for (int i = 0; i < n; i++)
			{
				if (sum < std::abs(inp[i]))
				{
					sum = std::abs(inp[i]);
				}
			}
			sum /= 32760.0; // make an int16 range
			break;
		case 2: // euclidean
			for (int i = 0; i < n; i++)
			{
				sum += inp[i] * inp[i];
			}
			sum = std::sqrt(sum);
			break;
		default: // p-norm (euclidean is p-norm p=2)
			for (int i = 0; i < n; i++)
			{
				sum += std::pow(std::abs(inp[i]), embd_norm);
			}
			sum = std::pow(sum, 1.0 / embd_norm);
			break;
		}

		const float norm = (sum > 0.0) ? 1.0f / toF(sum) : 0.0f;

		for (int i = 0; i < n; i++)
		{
			out[i] = inp[i] * norm;
		}
	}

	float embd_similarity_cos(const std::vector<float>& embd1, const std::vector<float>& embd2, int n)
	{
		double sum = 0.0;
		double sum1 = 0.0;
		double sum2 = 0.0;

		for (int i = 0; i < n; i++)
		{
			sum += embd1[i] * embd2[i];
			sum1 += embd1[i] * embd1[i];
			sum2 += embd2[i] * embd2[i];
		}

		// Handle the case where one or both vectors are zero vectors
		if (sum1 == 0.0 || sum2 == 0.0)
		{
			if (sum1 == 0.0 && sum2 == 0.0)
			{
				return 1.0f; // two zero vectors are similar
			}
			return 0.0f;
		}

		return toF(sum / (sqrt(sum1) * sqrt(sum2)));
	}

	PromptTemplateType _current_template = PromptTemplateType::Default;
	fig::string _template {};

	static const std::map<PromptTemplateType, fig::string> LLAMA_CPP_TEMPLATES = {
		{ PromptTemplateType::ChatML,			"chatml",			},
		{ PromptTemplateType::Llama2_v1,		"llama2",			},
		{ PromptTemplateType::Llama2_sys,		"llama2-sys",		},
		{ PromptTemplateType::Llama2_sys_bos,	"llama2-sys-bos",	},
		{ PromptTemplateType::Llama2_sys_strip,	"llama2-sys-strip",	},
		{ PromptTemplateType::Llama3,			"llama3",			},
		{ PromptTemplateType::Llama4,			"llama4",			},
		{ PromptTemplateType::Deepseek,			"deepseek",			},
		{ PromptTemplateType::Deepseek2,		"deepseek2",		},
		{ PromptTemplateType::Deepseek3,		"deepseek3",		},
		{ PromptTemplateType::Gemma,			"gemma",			},	// only: user, model

		{ PromptTemplateType::MistralV1,		"mistral-v1",		},
		{ PromptTemplateType::MistralV3,		"mistral-v3",		},
		{ PromptTemplateType::MistralV3_tekken,	"mistral-v3-tekken",},
		{ PromptTemplateType::MistralV7,		"mistral-v7",		},
		{ PromptTemplateType::Phi3,				"phi3",				},
		{ PromptTemplateType::Phi4,				"phi4",				},
		{ PromptTemplateType::CommandR,			"command-r",		},

		/* Not supported for now:
		{ PromptTemplateType::Vicuna,			"vicuna",			},
		{ PromptTemplateType::VicunaOrca,		"vicuna-orca",		},
		{ PromptTemplateType::Falcon3,			"falcon3",			},
		{ PromptTemplateType::Zephyr,			"zephyr",			},
		{ PromptTemplateType::Monarch,			"monarch",			},
		{ PromptTemplateType::Orion,			"orion",			},
		{ PromptTemplateType::OpenChat,			"openchat",			},
		{ PromptTemplateType::Chatglm3,			"chatglm3",			},
		{ PromptTemplateType::Chatglm4,			"chatglm4",			},
		{ PromptTemplateType::Glmedge,			"glmedge",			},
		{ PromptTemplateType::Minicpm,			"minicpm",			},
		{ PromptTemplateType::Exaone3,			"exaone3",			},
		{ PromptTemplateType::Rwkv_world,		"rwkv-world",		},
		{ PromptTemplateType::Granite,			"granite",			},
		{ PromptTemplateType::Gigachat,			"gigachat",			},
		{ PromptTemplateType::Megrez,			"megrez",			},
		{ PromptTemplateType::Yandex,			"yandex",			},
		{ PromptTemplateType::Bailing,			"bailing",			},
		{ PromptTemplateType::Smolvlm,			"smolvlm",			},
		*/
	};

#define LU8(x) (const char*)(u8##x)

	static const char* get_tmpl(PromptTemplateType tmpl)
	{
		return LLAMA_CPP_TEMPLATES.find(tmpl)->second.c_str();
	}

	// Lifted from llama.cpp API
	static int32_t apply_template(PromptTemplateType tmpl, const std::vector<const llama_chat_message*>& chat, fig::string& dest, bool add_ass)
	{
		// Taken from the research: https://github.com/ggerganov/llama.cpp/issues/5527
		std::stringstream ss;
		if (tmpl == PromptTemplateType::ChatML)
		{
			// chatml template
			for (auto message : chat)
			{
				ss << "<|im_start|>" << message->role << "\n" << message->content << "<|im_end|>\n";
			}
			if (add_ass)
			{
				ss << "<|im_start|>assistant\n";
			}
		}
		else if (tmpl == PromptTemplateType::MistralV7)
		{
			// Official mistral 'v7' template
			// See: https://huggingface.co/mistralai/Mistral-Large-Instruct-2411#basic-instruct-template-v7
			for (auto message : chat)
			{
				fig::string role(message->role);
				fig::string content(message->content);
				if (role == "system")
				{
					ss << "[SYSTEM_PROMPT] " << content << "[/SYSTEM_PROMPT]";
				}
				else if (role == "user")
				{
					ss << "[INST] " << content << "[/INST]";
				}
				else
				{
					ss << " " << content << "</s>";
				}
			}
		}
		else if (tmpl == PromptTemplateType::MistralV1
			|| tmpl == PromptTemplateType::MistralV3
			|| tmpl == PromptTemplateType::MistralV3_tekken)
		{
			// See: https://github.com/mistralai/cookbook/blob/main/concept-deep-dive/tokenization/chat_templates.md
			// See: https://github.com/mistralai/cookbook/blob/main/concept-deep-dive/tokenization/templates.md
			fig::string leading_space = tmpl == PromptTemplateType::MistralV1 ? " " : "";
			fig::string trailing_space = tmpl == PromptTemplateType::MistralV3_tekken ? "" : " ";
			bool trim_assistant_message = tmpl == PromptTemplateType::MistralV3;
			bool is_inside_turn = false;
			for (auto message : chat)
			{
				if (!is_inside_turn)
				{
					ss << leading_space << "[INST]" << trailing_space;
					is_inside_turn = true;
				}
				fig::string role(message->role);
				fig::string content(message->content);
				if (role == "system")
				{
					ss << content << "\n\n";
				}
				else if (role == "user")
				{
					ss << content << leading_space << "[/INST]";
				}
				else
				{
					ss << trailing_space << (trim_assistant_message ? trim(content) : content) << "</s>";
					is_inside_turn = false;
				}
			}
		}
		else if (
			tmpl == PromptTemplateType::Llama2_v1
			|| tmpl == PromptTemplateType::Llama2_sys
			|| tmpl == PromptTemplateType::Llama2_sys_bos
			|| tmpl == PromptTemplateType::Llama2_sys_strip)
		{
			// llama2 template and its variants
			// [variant] support system message
			// See: https://huggingface.co/blog/llama2#how-to-prompt-llama-2
			bool support_system_message = tmpl != PromptTemplateType::Llama2_v1;
			// [variant] add BOS inside history
			bool add_bos_inside_history = tmpl == PromptTemplateType::Llama2_sys_bos;
			// [variant] trim spaces from the input message
			bool strip_message = tmpl == PromptTemplateType::Llama2_sys_strip;
			// construct the prompt
			bool is_inside_turn = true; // skip BOS at the beginning
			ss << "[INST] ";
			for (auto message : chat)
			{
				fig::string content = strip_message ? trim(message->content) : message->content;
				fig::string role(message->role);
				if (!is_inside_turn)
				{
					is_inside_turn = true;
					ss << (add_bos_inside_history ? "<s>[INST] " : "[INST] ");
				}
				if (role == "system")
				{
					if (support_system_message)
					{
						ss << "<<SYS>>\n" << content << "\n<</SYS>>\n\n";
					}
					else
					{
						// if the model does not support system message, we still include it in the first message, but without <<SYS>>
						ss << content << "\n";
					}
				}
				else if (role == "user")
				{
					ss << content << " [/INST]";
				}
				else
				{
					ss << content << "</s>";
					is_inside_turn = false;
				}
			}
		}
		else if (tmpl == PromptTemplateType::Phi3)
		{
			// Phi 3
			for (auto message : chat)
			{
				fig::string role(message->role);
				ss << "<|" << role << "|>\n" << message->content << "<|end|>\n";
			}
			if (add_ass)
			{
				ss << "<|assistant|>\n";
			}
		}
		else if (tmpl == PromptTemplateType::Phi4)
		{
			// chatml template
			for (auto message : chat)
			{
				ss << "<|im_start|>" << message->role << "<|im_sep|>" << message->content << "<|im_end|>";
			}
			if (add_ass)
			{
				ss << "<|im_start|>assistant<|im_sep|>";
			}
		}
		else if (tmpl == PromptTemplateType::Gemma)
		{
			// google/gemma-7b-it
			fig::string system_prompt = "";
			for (auto message : chat)
			{
				fig::string role(message->role);
				if (role == "system")
				{
					// there is no system message for gemma, but we will merge it with user prompt, so nothing is broken
					if (chat.size() > 1)
					{
						system_prompt = trim(message->content);
						continue;
					}
					else
						role = "user";
				}
				// in gemma, "assistant" is "model"
				role = role == "assistant" ? "model" : message->role;
				ss << "<start_of_turn>" << role << "\n";
				if (!system_prompt.empty() && role != "model")
				{
					ss << system_prompt << "\n\n";
					system_prompt = "";
				}
				ss << trim(message->content) << "<end_of_turn>\n";
			}
			if (add_ass)
			{
				ss << "<start_of_turn>model\n";
			}
		}
		else if (tmpl == PromptTemplateType::Deepseek)
		{
			// deepseek-ai/deepseek-coder-33b-instruct
			for (auto message : chat)
			{
				fig::string role(message->role);
				if (role == "system")
				{
					ss << message->content;
				}
				else if (role == "user")
				{
					ss << "### Instruction:\n" << message->content << "\n";
				}
				else if (role == "assistant")
				{
					ss << "### Response:\n" << message->content << "\n<|EOT|>\n";
				}
			}
			if (add_ass)
			{
				ss << "### Response:\n";
			}
		}
		else if (tmpl == PromptTemplateType::CommandR)
		{
			// CohereForAI/c4ai-command-r-plus
			for (auto message : chat)
			{
				fig::string role(message->role);
				if (role == "system")
				{
					ss << "<|START_OF_TURN_TOKEN|><|SYSTEM_TOKEN|>" << trim(message->content) << "<|END_OF_TURN_TOKEN|>";
				}
				else if (role == "user")
				{
					ss << "<|START_OF_TURN_TOKEN|><|USER_TOKEN|>" << trim(message->content) << "<|END_OF_TURN_TOKEN|>";
				}
				else if (role == "assistant")
				{
					ss << "<|START_OF_TURN_TOKEN|><|CHATBOT_TOKEN|>" << trim(message->content) << "<|END_OF_TURN_TOKEN|>";
				}
			}
			if (add_ass)
			{
				ss << "<|START_OF_TURN_TOKEN|><|CHATBOT_TOKEN|>";
			}
		}
		else if (tmpl == PromptTemplateType::Llama3)
		{
			// Llama 3
			for (auto message : chat)
			{
				fig::string role(message->role);
				ss << "<|start_header_id|>" << role << "<|end_header_id|>\n\n" << trim(message->content) << "<|eot_id|>";
			}
			if (add_ass)
			{
				ss << "<|start_header_id|>assistant<|end_header_id|>\n\n";
			}
		}
		else if (tmpl == PromptTemplateType::Deepseek2)
		{
			// DeepSeek-V2
			for (auto message : chat)
			{
				fig::string role(message->role);
				if (role == "system")
				{
					ss << message->content << "\n\n";
				}
				else if (role == "user")
				{
					ss << "User: " << message->content << "\n\n";
				}
				else if (role == "assistant")
				{
					ss << "Assistant: " << message->content << LU8("<｜end▁of▁sentence｜>");
				}
			}
			if (add_ass)
			{
				ss << "Assistant:";
			}
		}
		else if (tmpl == PromptTemplateType::Deepseek3)
		{
			// DeepSeek-V3
			for (auto message : chat)
			{
				fig::string role(message->role);
				if (role == "system")
				{
					ss << message->content << "\n\n";
				}
				else if (role == "user")
				{
					ss << LU8("<｜User｜>") << message->content;
				}
				else if (role == "assistant")
				{
					ss << LU8("<｜Assistant｜>") << message->content << LU8("<｜end▁of▁sentence｜>");
				}
			}
			if (add_ass)
			{
				ss << LU8("<｜Assistant｜>");
			}
		}
		else if (tmpl == PromptTemplateType::Llama4)
		{
			// Llama 4
			for (auto message : chat)
			{
				fig::string role(message->role);
				ss << "<|header_start|>" << role << "<|header_end|>\n\n" << trim(message->content) << "<|eot|>";
			}
			if (add_ass)
			{
				ss << "<|header_start|>assistant<|header_end|>\n\n";
			}
		}
		else
		{
			// template not supported
			return -1;
		}
		dest = ss.str();
		return toI(dest.size());
	}

	std::pair<fig::string, fig::string> get_chat_template_prefix_suffix(Role role, fig::string name)
	{
		// Strip prompt template from block content
		static const char* const SUBSTITUTE = "{{SUBSTITUTE}}";
		fig::string tmpl = apply_chat_template({ Message { role, SUBSTITUTE, name } }, false);
		if (tmpl.empty())
			return std::make_pair("", ""); // Unknown template

		size_t pos_msg = tmpl.find(SUBSTITUTE);
		fig::string prefix = tmpl.substr(0, pos_msg);
		fig::string suffix = tmpl.substr(pos_msg + strlen(SUBSTITUTE));
		return std::make_pair(prefix, suffix);
	}

	fig::string apply_chat_template_prefix(Role role, fig::string content, fig::string name)
	{
		auto [pre, post] = get_chat_template_prefix_suffix(role, name);

		if (ends_with(content, post))
			content = content.substr(0, content.length() - post.length());
		if (!begins_with(content, pre))
			content = pre + content;
		return content;
	}

	PromptTemplateType auto_detect_template(llama_model* pModel)
	{
		const char* tmpl = llama_model_chat_template(pModel, nullptr);
		if (tmpl)
		{
			_template = toStr(tmpl);
			_current_template = PromptTemplateType::Automatic;
			return PromptTemplateType::Automatic;
		}

		// Not found
		_current_template = PromptTemplateType::Undefined;
		return PromptTemplateType::Undefined;
	}

	fig::string apply_chat_template(Messages messages, bool add_assistant)
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
			std::vector<char> formatted(Constants::DefaultModelSettings::MaxResponseLength * 2);
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

			return fig::string(formatted.begin(), formatted.begin() + new_len);
		}
		else
		{
			std::vector<const llama_chat_message*> pMsgs;
			pMsgs.reserve(messages.size());
			for (size_t i = 0; i < msgs.size(); ++i)
				pMsgs.push_back(&msgs[i]);

			PromptTemplateType tmpl = _current_template;
			if (tmpl == PromptTemplateType::Undefined || tmpl == PromptTemplateType::Automatic)
				tmpl = PromptTemplateType::Default;

			fig::string formatted;
			int new_len = apply_template(tmpl, pMsgs, formatted, add_assistant);

			if (new_len < 0)
			{
				assert(0 && "failed to apply the chat template");
				return "";
			}
			return formatted;
		}

	}




} // namespace