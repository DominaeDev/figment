#include "Utility.h"

#include <algorithm> 
#include <cctype>
#include <locale>
#include <fstream>


SDL_FRect Rect_Expand(const SDL_FRect& rect, float pixels)
{
	return SDL_FRect { rect.x - pixels, rect.y - pixels, rect.w + pixels * 2, rect.h + pixels * 2 };
}

std::optional<string> ReadTextFile(const string& filename, bool normalizeNewlines)
{
	try
	{
		std::ifstream file(filename.c_str(), std::ios::binary | std::ios::in | std::ios::ate);
		if (!file)
			return std::nullopt;

		std::streamsize size = file.tellg();
		file.seekg(0, std::ios::beg);

		string content;
		content.reserve(size);
		content.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
		return normalizeNewlines ? NormalizeNewlines(content) : content;
	}
	catch (...)
	{
		return std::nullopt;
	}
}

bool ReadTextFile(const string& filename, string& out_content, bool normalizeNewlines)
{
	auto content = ReadTextFile(filename, normalizeNewlines);
	if (content.has_value())
	{
		out_content = content.value();
		return true;
	}
	return false;
}


bool WriteTextFile(const string& filename, const string& content, bool append)
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

string& NormalizeNewlines(string& text)
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

string NormalizeNewlines(string&& text)
{
	return NormalizeNewlines(text); // rvo
}

void DebugPrint(string message)
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

void DebugPrintLn(string message)
{
	DebugPrint(message);
	DebugPrint("\r\n");
}

string CreateUUID()
{
	static UUIDv4::UUIDGenerator<std::mt19937_64> uuidGenerator;
	return uuidGenerator.getUUID().str();
}
