#include <pch.h>
#include "chat/ChatFormatter.h"

namespace fig::chat
{
	namespace Tags
	{
		namespace Open
		{
			constexpr fig::const_string Dialogue = "dialogue";
			constexpr fig::const_string Action = "emote";
			constexpr fig::const_string Thought = "thinking";
			constexpr fig::const_string Narration = "narrator";
			constexpr fig::const_string Direction = "director";
		}

		namespace Close
		{
			constexpr fig::const_string Dialogue = "/dialogue";
			constexpr fig::const_string Action = "/emote";
			constexpr fig::const_string Thought = "/thinking";
			constexpr fig::const_string Narration = "/narrator";
			constexpr fig::const_string Direction = "/director";
		}
		
		constexpr fig::const_string StateReport = "change";
	}

	static std::vector<fig::string> const opening_tags {
		std::format("<{0}=\"", Tags::Open::Dialogue),
		std::format("<{0}=\"", Tags::Open::Action),
		std::format("<{0}=\"", Tags::Open::Thought),
		std::format("<{0}>",   Tags::Open::Narration),
		std::format("<{0}>",   Tags::Open::Direction),
	};

	static std::vector<fig::string> const closing_tags {
		std::format("</{0}>", Tags::Close::Dialogue),
		std::format("</{0}>", Tags::Close::Action),
		std::format("</{0}>", Tags::Close::Thought),
		std::format("</{0}>", Tags::Close::Narration),
		std::format("</{0}>", Tags::Close::Direction),
	};

	fig::string_list ChatFormatter::GetOpeningTags() noexcept
	{
		return opening_tags;
	}

	fig::string_list ChatFormatter::GetClosingTags() noexcept
	{
		return closing_tags;
	}

	fig::string_list ChatFormatter::GetFormattingTags() noexcept
	{
		fig::string_list tags;
		tags.reserve(opening_tags.size() + closing_tags.size());
		tags.insert(tags.end(), opening_tags.begin(), opening_tags.end());
		tags.insert(tags.end(), closing_tags.begin(), closing_tags.end());
		return tags;
	}

	std::pair<fig::string_view, fig::string_view> ChatFormatter::GetMessageTags(MessageType msgType) noexcept
	{
		switch (msgType)
		{
		case MessageType::Dialogue:
			return std::pair { Tags::Open::Dialogue, Tags::Close::Dialogue };
		case MessageType::Action:
			return std::pair { Tags::Open::Action, Tags::Close::Action };
		case MessageType::Thought:
			return std::pair { Tags::Open::Thought, Tags::Close::Thought };
		case MessageType::Narration:
			return std::pair { Tags::Open::Narration, Tags::Close::Narration };
		case MessageType::Direction:
			return std::pair { Tags::Open::Direction, Tags::Close::Direction };
		default:
			return {};
		}
	}

	std::pair<fig::string, fig::string> ChatFormatter::GetTagAndName(fig::string_view text)
	{
		fig::string tag;
		size_t pos_equals = text.find('=', 1);
		if (pos_equals == fig::npos)
		{
			tag = trim(text.substr(1, text.length() - 2));
			return std::pair { tag, fig::string {} };
		}

		tag = trim(text.substr(1, pos_equals - 1));
		fig::string name = fig::string { trim(text.substr(pos_equals + 1, text.length() - pos_equals - 2)) };
		replace_all_inplace(name, "\"", "");
		return std::pair { tag, name };
	}

	MessageType ChatFormatter::DetectMessageType(fig::string_view text) noexcept
	{
		bool tmp;
		return DetectMessageType(text, tmp);
	}

	MessageType ChatFormatter::DetectMessageType(fig::string_view text, bool& out_bComplete) noexcept
	{
		auto patterns = std::vector<std::tuple<fig::string, MessageType>> {
			{ std::format("<{0}", Tags::Open::Dialogue),	MessageType::Dialogue },
			{ std::format("<{0}", Tags::Open::Action),		MessageType::Action },
			{ std::format("<{0}", Tags::Open::Thought),		MessageType::Thought },
			{ std::format("<{0}", Tags::Open::Narration),	MessageType::Narration },
			{ std::format("<{0}", Tags::Open::Direction),	MessageType::Direction },
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

		out_bComplete = false;
		for (auto& tag : closing_tags)
		{
			size_t pos_end = text.rfind(tag, fig::npos);
			if (pos_end != fig::npos && pos_end > last_pos)
			{
				out_bComplete = true;
				break;
			}
		}

		return msgType;
	}

	fig::string& ChatFormatter::SanitizeMessage(fig::string& text)
	{
		size_t pos_last = text.find_last_of('<');
		if (pos_last == fig::npos)
			return text;
		size_t pos_close = text.find_last_of('>');
		if (pos_close == fig::npos || pos_close < pos_last)
			text = text.substr(0, pos_last);
		return text;
	}

	fig::string& ChatFormatter::CompleteMessage(fig::string& text)
	{
		bool bComplete;
		auto msgType = DetectMessageType(SanitizeMessage(text), bComplete);
		if (bComplete)
			return text;

		switch (msgType)
		{
		case MessageType::Undefined:
			text = std::format("<{1}=\"{{character}}\">{0}</{2}>", text, Tags::Open::Dialogue, Tags::Close::Dialogue);
			break;
		case MessageType::Dialogue:
			text.append(std::format("\"<{0}>", Tags::Close::Dialogue));
			break;
		case MessageType::Action:
			text.append(std::format("*<{0}>", Tags::Close::Action));
			break;
		case MessageType::Thought:
			text.append(std::format("))<{0}>", Tags::Close::Thought));
			break;
		case MessageType::Narration:
			text.append(std::format("]<{0}>", Tags::Close::Narration));
			break;
		case MessageType::Direction:
			text.append(std::format("<{0}>", Tags::Close::Direction));
			break;
		case MessageType::SystemMessage:
		default:
			break;
		}
		return text;
	}

	MessageType ChatFormatter::GetMessageTypeForTag(fig::string_view tag)
	{
		if (tag == Tags::Open::Dialogue)
			return MessageType::Dialogue;
		else if (tag == Tags::Open::Action)
			return MessageType::Action;
		else if (tag == Tags::Open::Thought)
			return MessageType::Thought;
		else if (tag == Tags::Open::Narration)
			return MessageType::Narration;
		else if (tag == Tags::Open::Direction)
			return MessageType::Direction;
		else if (tag == Tags::StateReport)
			return MessageType::StateReport;
		return MessageType::Undefined;
	}

	void ChatFormatter::StripEndTag(fig::string& message) noexcept
	{
		size_t pos_end = message.rfind("</", fig::npos);
		if (pos_end != fig::npos)
		{
			message = message.substr(0, pos_end);
			char last_char = message.back();
			if (last_char == '*' || last_char == '"' || last_char == ']')
				message.pop_back(); // Trim scaffolding char
		}
	}

	fig::string ChatFormatter::FormatMessage(fig::string_view message, fig::string_view identifier, MessageType msgType) noexcept
	{
		switch (msgType)
		{
		case MessageType::Dialogue:
			return std::format("<{2}=\"{0}\">{1}<{3}>", identifier, Dialogue(message), Tags::Open::Dialogue, Tags::Close::Dialogue);
			break;
		case MessageType::Action:
			return std::format("<{2}=\"{0}\">{1}<{3}>", identifier, Action(message), Tags::Open::Action, Tags::Close::Action);
			break;
		case MessageType::Thought:
			return std::format("<{2}=\"{0}\">{1}<{3}>", identifier, Thought(message), Tags::Open::Thought, Tags::Close::Thought);
			break;
		case MessageType::Narration:
			return std::format("<{1}>{0}<{2}>", message, Narration(Tags::Open::Narration), Tags::Close::Narration);
			break;
		case MessageType::Direction:
			return std::format("<{1}>{0}<{2}>", message, Director(Tags::Open::Direction), Tags::Close::Direction);
		}
		return {};
	}

	fig::string ChatFormatter::Dialogue(fig::string_view text) noexcept
	{
		return std::format("\"{}\"", text);
	}

	fig::string ChatFormatter::Action(fig::string_view text) noexcept
	{
		return std::format("*{}*", text);
	}

	fig::string ChatFormatter::Thought(fig::string_view text) noexcept
	{
		return std::format("(({}))", text);
	}

	fig::string ChatFormatter::Director(fig::string_view text) noexcept
	{
		return std::format("\\{{\\{{{}\\}}\\}}", text);
	}

	fig::string ChatFormatter::Narration(fig::string_view text) noexcept
	{
		return std::format("[{}]", text);
	}

	fig::string ChatFormatter::Wrap(fig::string_view text, MessageType msgType) noexcept
	{
		switch (msgType)
		{
		case MessageType::Dialogue:
			return Dialogue(text);
		case MessageType::Action:
			return Action(text);
		case MessageType::Thought:
			return Thought(text);
		case MessageType::Narration:
			return Narration(text);
		case MessageType::Direction:
			return Director(text);
		default:
			return fig::string { text };
		}
	}

	fig::string_view ChatFormatter::Unwrap(fig::string_view text) noexcept
	{
		if (text.length() > 2 and text.front() == '\"' and text.back() == '\"')
			return text.substr(1, text.length() - 2);
		if (text.length() > 2 and text.front() == '*' and text.back() == '*')
			return text.substr(1, text.length() - 2);
		if (text.length() > 2 and text.front() == '[' and text.back() == ']')
			return text.substr(1, text.length() - 2);
		if (text.length() > 4 and text[0] == '{' and text[1] == '{' and text[text.length() - 1] == '}' and text[text.length() - 2] == '}')
			return text.substr(2, text.length() - 4);
		if (text.length() > 4 and text[0] == '(' and text[1] == '(' and text[text.length() - 1] == ')' and text[text.length() - 2] == ')')
			return text.substr(2, text.length() - 4);
		return text;
	}

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

	std::pair<fig::string, std::vector<Submessage>> ChatFormatter::ProcessMessage(fig::string_view message, fig::string_view identifier) noexcept
	{
		size_t pos = 0;
		size_t length = message.size();

		fig::string sMessage { trim(message) };
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
			size_t pos_last = sMessage.find(last, 1);
			if (pos_last == fig::npos)
				sMessage += last;
		}

		std::vector<Span> spans;
		spans.reserve(64);

		mark_spans(sMessage, MessageType::Dialogue, "\"", "\"", spans);
		mark_spans(sMessage, MessageType::Action, "*", "*", spans);
		mark_spans(sMessage, MessageType::Narration, "[", "]", spans);
		mark_spans(sMessage, MessageType::Thought, "((", "))", spans);
		mark_spans(sMessage, MessageType::Direction, "{{", "}}", spans);

		fill_gaps(sMessage, MessageType::Dialogue, spans);
		trim_spans(sMessage, spans);

		std::vector<Submessage> subMessages;
		fig::string result;
		result.reserve(256);
		for (auto& span : spans)
		{
			fig::string text = sMessage.substr(span.start, span.end - span.start);
			text = Unwrap(trim(text));

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

			result.append(ChatFormatter::FormatMessage(text, identifier, span.msgType));

			subMessages.push_back(Submessage {
				span.msgType,
				text,
			});
		}

		return std::pair { std::move(result), std::move(subMessages) };
	}

	std::vector<Submessage> ChatFormatter::AnalyseMessage(fig::string_view message) noexcept
	{
		std::vector<Submessage> subMessages;
		auto paragraphs = split_paragraphs(message);

		for (auto& paragraph : paragraphs)
		{
			fig::string sMessage { paragraph };
			char first = sMessage[0];
			char last = 0;
			if (first == '"' || first == '*')
				last = first;
			if (last != 0)
			{
				size_t pos_last = sMessage.find(last, 1);
				if (pos_last == fig::npos)
					sMessage += last;
			}

			std::vector<Span> spans;
			spans.reserve(64);

 			mark_spans(sMessage, MessageType::Dialogue, "\"", "\"", spans);
			mark_spans(sMessage, MessageType::Action, "*", "*", spans);
			fill_gaps(sMessage, MessageType::Narration, spans);
			trim_spans(sMessage, spans);

			for (auto& span : spans)
			{
				fig::string text = sMessage.substr(span.start, span.end - span.start);
				text = Unwrap(trim(text));
				if (empty_or_whitespace(text))
					continue;

				subMessages.push_back(Submessage {
					span.msgType,
					text,
				});
			}
		}

		// Combine submessages of same type
		size_t index_write = 0;
		for (size_t index_read = 1; index_read < subMessages.size(); index_read++)
		{
			if (subMessages[index_write].msgType == subMessages[index_read].msgType)
			{
				subMessages[index_write].content += "\n\n";
				subMessages[index_write].content += subMessages[index_read].content;
			}
			else
			{
				index_write++;
				if (index_write != index_read)
					subMessages[index_write] = std::move(subMessages[index_read]);
			}
		}
		if (not subMessages.empty())
			subMessages.erase(subMessages.begin() + index_write + 1, subMessages.end());
		return subMessages;
	}

	fig::string ChatFormatter::CreateMessagePrefix(fig::string_view identifier, MessageType msgType) noexcept
	{
		if (msgType == MessageType::Dialogue)
			return std::format("<{0}=\"{1}\">", Tags::Open::Dialogue, identifier);
		else if (msgType == MessageType::Action)
			return std::format("<{0}=\"{1}\">", Tags::Open::Action, identifier);
		else if (msgType == MessageType::Thought)
			return std::format("<{0}=\"{1}\">", Tags::Open::Thought, identifier);
		else if (msgType == MessageType::Narration)
			return std::format("<{0}>", Tags::Open::Narration);
		else if (msgType == MessageType::Direction)
			return std::format("<{0}>", Tags::Open::Direction);
		return "";
	}
}