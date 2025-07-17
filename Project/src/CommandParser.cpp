#include "CommandParser.h"
#include "StringUtil.h"

struct _Cmd {
	string word;
	CommandType cmdType;
};

static _Cmd s_Commands[] {
	{ "say",			CommandType::UserMessage },
	{ "system",			CommandType::SystemMessage },
	{ "pass",			CommandType::PassTurn},
	{ "talk",			CommandType::InstigateDialogue},
	{ "act",			CommandType::InstigateAction},
	{ "narrate",		CommandType::Narrate },
	{ "guide",			CommandType::Guide },
	{ "reset",			CommandType::Reset },
	{ "remove",			CommandType::RemoveLast},
	{ "undo",			CommandType::RollbackUserMessage },
	{ "retry",			CommandType::RedoResponse },
	{ "reseed",			CommandType::Reseed },
	{ "impersonate",	CommandType::Impersonate },
	{ "examine",		CommandType::Examine },
	{ "look",			CommandType::Look },
};

Command CommandParser::Parse(string text)
{
	trim(text);
	if (text.empty())
		return Command { CommandType::Invalid };

	if (text == "..." || text == ".." || text == ".") // Shorthand
		return Command { CommandType::InstigateDialogue };

	if (text[0] != '/')
		return Command { CommandType::UserMessage, text };

	if (string_begins_with(text, "//")) // Shorthand
	{
		string payload = trim(text.substr(2));
		return Command { CommandType::SystemMessage, payload };
	}

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