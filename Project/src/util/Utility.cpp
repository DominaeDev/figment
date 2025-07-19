#include "util/Utility.h"
#include "util/StringUtility.h"
#include "Constants.h"

#include <algorithm> 
#include <cctype>
#include <locale>
#include <fstream>
#include <uuid_v4.h>
#include <format>

std::optional<std::string> ReadTextFile(const std::string& filename, bool normalizeNewlines)
{
	try
	{
		std::ifstream file(filename.c_str(), std::ios::binary | std::ios::in | std::ios::ate);
		if (!file)
			return std::nullopt;

		std::streamsize size = file.tellg();
		file.seekg(0, std::ios::beg);

		std::string content;
		content.reserve(size);
		content.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
		return normalizeNewlines ? NormalizeNewlines(content) : content;
	}
	catch (...)
	{
		return std::nullopt;
	}
}

bool ReadTextFile(const std::string& filename, std::string& out_content, bool normalizeNewlines)
{
	auto content = ReadTextFile(filename, normalizeNewlines);
	if (content.has_value())
	{
		out_content = content.value();
		return true;
	}
	return false;
}


bool WriteTextFile(const std::string& filename, const std::string& content, bool append)
{
	try
	{
		std::ofstream file(filename.c_str(), std::ios::binary | std::ios::out | (append ? std::ios::app : std::ios::trunc));
		if (!file.is_open())
			return false;

		file.write(content.c_str(), content.length());
		return !file.fail();
	}
	catch (...)
	{
		return false;
	}
}

std::string& NormalizeNewlines(std::string& text)
{
	size_t cursor_write = 0;

	for (size_t cursor_read = 0; cursor_read < text.size(); ++cursor_read)
	{
		if (text[cursor_read] == '\r')
		{
			// Skip CR and optional following LF
			if (cursor_read + 1 < text.size() && text[cursor_read + 1] == '\n')
				++cursor_read;
			text[cursor_write++] = '\n';
		}
		else
			text[cursor_write++] = text[cursor_read];
	}

	text.resize(cursor_write);
	return text;
}

std::string NormalizeNewlines(std::string&& text)
{
	return NormalizeNewlines(text); // rvo
}

void DebugPrint(std::string message)
{
#if _DEBUG
	if (message.empty())
		return;

	printf(message.c_str());
	fflush(stdout);
#else
	// noop
#endif
}

void DebugPrintLn(std::string message)
{
	DebugPrint(message);
	DebugPrint("\r\n");
}

std::string CreateUUID()
{
	static UUIDv4::UUIDGenerator<std::mt19937_64> uuidGenerator;
	return uuidGenerator.getUUID().str();
}

enum class SpanType
{
	Undefined = -1, 
	Dialogue = 0,
	QuotedDialogue,
	Action,
	Thought,
	Narration,
	Direction,
};

struct Span
{
	SpanType msgType;
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

static void mark_spans(const std::string s, SpanType type, std::string open, std::string close, std::vector<Span>& spans)
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

		spans.push_back(Span { type, pos_open, pos_close + close.size() });
		pos_open = find_next(s, open, pos_close + close.size());
	}
}

static void fill_gaps(const std::string s, SpanType type, std::vector<Span>& spans)
{
	auto CheckAndAdd = [type, &spans](size_t pos, size_t len) {
		if (len != 0)
			spans.push_back(Span { type, pos, pos + len });
		return true;
	};

	// Sort spans
	std::sort(std::begin(spans), std::end(spans), [](Span a, Span b) {
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

	for (int i = 0; i < n; ++i)
	{
		auto& first = spans[i];
		auto& second = spans[i + 1];
		if (first.end < second.start)
			CheckAndAdd(first.end, second.start - first.end);
	}

	// Sort again
	std::sort(std::begin(spans), std::end(spans), [](Span a, Span b) {
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

std::string FormatMessage(std::string message, std::string actorName)
{
	size_t pos = 0;
	size_t length = message.size();

	string_util::trim(message);
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

	mark_spans(message, SpanType::QuotedDialogue, "\"", "\"", spans);
	mark_spans(message, SpanType::Action, "*", "*", spans);
	mark_spans(message, SpanType::Thought, "((", "))", spans);
	mark_spans(message, SpanType::Narration, "[", "]", spans);
	mark_spans(message, SpanType::Direction, "{{", "}}", spans);

	fill_gaps(message, SpanType::Dialogue, spans);
//	trim_spans(message, spans);

	std::string result;
	result.reserve(256);
	for (auto& span : spans)
	{
		std::string text = message.substr(span.start, span.end - span.start);

		switch (span.msgType)
		{
		case SpanType::QuotedDialogue:
		case SpanType::Action:
		case SpanType::Narration:
			text.erase(text.length() - 1, 1);
			text.erase(0, 1);
			break;
		case SpanType::Thought:
		case SpanType::Direction:
			text.erase(text.length() - 2, 2);
			text.erase(0, 2);
			break;
		}
		
		if (string_util::trim(text).empty())
			continue;

		if (result.length() > 0)
			result.append(" ");

		switch (span.msgType)
		{
		case SpanType::QuotedDialogue:
		case SpanType::Dialogue:
			result.append(std::format("<{0}=\"{1}\">\"{2}\"</{0}>", Constants::DialogueTag, actorName, text));
			break;
		case SpanType::Action:
			result.append(std::format("<{0}=\"{1}\">*{2}*</{0}>", Constants::ActionTag, actorName, text));
			break;
		case SpanType::Thought:
			result.append(std::format("<{0}=\"{1}\">({2})</{0}>", Constants::ThoughtTag, actorName, text));
			break;
		case SpanType::Narration:
			result.append(std::format("<{0}>[{1}]</{0}>", Constants::NarrationTag, text));
			break;
		case SpanType::Direction:
			result.append(std::format("<{0}>{1}</{0}>", Constants::DirectionTag, text));
			break;
		}
	}

	return result;
}