#include "util/CommandParser.h"
#include "util/StringUtility.h"

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
	{ "instruct",		CommandType::Instruct },
	{ "reset",			CommandType::Reset },
	{ "erase",			CommandType::RemoveLast},
	{ "undo",			CommandType::RollbackUserMessage },
	{ "retry",			CommandType::RedoResponse },
	{ "reseed",			CommandType::Reseed },
	{ "impersonate",	CommandType::Impersonate },
	{ "examine",		CommandType::Examine },
	{ "look",			CommandType::Look },
};

Command CommandParser::Parse(string text)
{
	string_util::trim_str(text);
	if (text.empty())
		return Command { CommandType::Invalid };

	if (text == "..." || text == ".." || text == ".") // Shorthand
		return Command { CommandType::InstigateDialogue };

	if (text[0] != '/')
		return Command { CommandType::UserMessage, text };

	if (string_util::begins_with(text, "//")) // Shorthand
	{
		string payload = string_util::trim(text.substr(2));
		return Command { CommandType::SystemMessage, payload };
	}

	size_t pos_begin = text.find(' ');
	string command;
	string payload;
	if (pos_begin != std::string::npos)
	{
		command = string_util::lcase(string_util::trim(text.substr(1, pos_begin - 1)));
		payload = string_util::trim(text.substr(pos_begin));
	}
	else
	{
		command = string_util::lcase(string_util::trim(text.substr(1, pos_begin - 1)));
		payload = "";
	}

	for (auto& c : s_Commands)
	{
		if (c.word == command)
			return Command { c.cmdType, payload };
	}

	return Command { CommandType::Invalid };
}