module;

#include <string>

export module Utility.Command;

import Utility;

using string = std::string;

export enum class CommandType
{
	Invalid,
	UserMessage,
	SystemMessage,
	PassTurn,
	InstigateDialogue,
	InstigateAction,
	Impersonate,
	Narrate,
	Instruct,
	Reset,
	RemoveLast,
	RollbackUserMessage,
	RedoResponse,
	Reseed,
	Look,
	Examine,
	GenerateEmbedding,
	NewStateVariable,
	SetStateVariable,
};

export struct Command
{
	CommandType type = CommandType::UserMessage;
	string text;
};

struct _Cmd {
	string word;
	CommandType cmdType;
};

_Cmd s_Commands[] {
	{ "act",			CommandType::InstigateAction },
	{ "erase",			CommandType::RemoveLast },
	{ "examine",		CommandType::Examine },
	{ "impersonate",	CommandType::Impersonate },
	{ "instruct",		CommandType::Instruct },
	{ "look",			CommandType::Look },
	{ "narrate",		CommandType::Narrate },
	{ "pass",			CommandType::PassTurn},
	{ "redo",			CommandType::RedoResponse },
	{ "reset",			CommandType::Reset },
	{ "reseed",			CommandType::Reseed },
	{ "say",			CommandType::UserMessage },
	{ "system",			CommandType::SystemMessage },
	{ "talk",			CommandType::InstigateDialogue},
	{ "undo",			CommandType::RollbackUserMessage },
	{ "embed",			CommandType::GenerateEmbedding },
	{ "define",			CommandType::NewStateVariable },
	{ "set",			CommandType::SetStateVariable },
};

export class CommandParser
{
public:
	static Command Parse(string text)
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

		// Partial match
		for (auto& c : s_Commands)
		{
			if (string_util::begins_with(c.word, command))
				return Command { c.cmdType, payload };
		}

		return Command { CommandType::Invalid };
	}
};
