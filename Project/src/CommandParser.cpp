#include "CommandParser.h"
#include "StringUtil.h"

struct _Cmd {
	string word;
	CommandType cmdType;
};

static _Cmd s_Commands[] {
	{ "say", CommandType::Say },
	{ "undo", CommandType::UndoMessage },
	{ "system", CommandType::SystemMessage },
	{ "reset", CommandType::Reset },
};

Command CommandParser::Parse(string text)
{
	trim(text);
	if (text.empty())
		return Command { CommandType::Invalid };

	if (text[0] != '/')
		return Command { CommandType::Say, text };

	size_t pos_begin = text.find(' ');
	string command;
	string payload;
	if (pos_begin != std::string::npos)
	{
		command = lcase(trim(text.substr(1, pos_begin - 1)));
		payload = trim(text.substr(pos_begin));
	}
	else
	{
		command = lcase(trim(text.substr(1, pos_begin - 1)));
		payload = "";
	}

	for (auto& c : s_Commands)
	{
		if (c.word == command)
			return Command { c.cmdType, payload };
	}

	return Command { CommandType::Invalid };
}