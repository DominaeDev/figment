#include <pch.h>
#include "model/ChatCommands.h"
#include "util/StringUtility.h"

using namespace fig::util;

struct CommandDefinition {
	fig::string keyword;
	ChatCommand command;
};

static CommandDefinition s_Commands[]
{
	{ "act",			ChatCommand::InstigateAction },
	{ "erase",			ChatCommand::Erase },
	{ "examine",		ChatCommand::Examine },
	{ "impersonate",	ChatCommand::Impersonate },
	{ "instruct",		ChatCommand::Instruct },
	{ "look",			ChatCommand::Look },
	{ "narrate",		ChatCommand::Narrate },
	{ "pass",			ChatCommand::PassTurn},
	{ "redo",			ChatCommand::RedoResponse },
	{ "reset",			ChatCommand::Reset },
	{ "reseed",			ChatCommand::Reseed },
	{ "say",			ChatCommand::UserMessage },
	{ "system",			ChatCommand::SystemMessage },
	{ "talk",			ChatCommand::InstigateDialogue },
	{ "reply",			ChatCommand::InstigateDialogue },
	{ "undo",			ChatCommand::RollbackUserMessage },
	{ "embed",			ChatCommand::GenerateEmbedding },
	{ "define",			ChatCommand::NewStateVariable },
	{ "set",			ChatCommand::SetStateVariable },
};

ParsedChatCommand ChatCommands::Parse(fig::string text)
{
	trim_inplace(text);
	if (text.empty())
	{
		return ParsedChatCommand { ChatCommand::Invalid };
	}

	if (text == "..." || text == ".." || text == ".") // Shorthand for continue
	{
		return ParsedChatCommand { ChatCommand::InstigateDialogue };
	}

	if (text[0] != '/')
	{
		return ParsedChatCommand 
		{ 
			.command = ChatCommand::UserMessage,
			.text = text 
		};
	}

	if (begins_with(text, "//")) // Shorthand
	{
		fig::string payload = trim(text.substr(2));
		return ParsedChatCommand 
		{ 
			.command = ChatCommand::SystemMessage, 
			.text = payload,
		};
	}

	size_t pos_begin = text.find(' ');
	fig::string command;
	fig::string payload;
	if (pos_begin != fig::npos)
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
		if (c.keyword == command)
		{
			return ParsedChatCommand {
				c.command,
				payload
			};
		}
	}
	
	// Partial match
	for (auto& c : s_Commands)
	{
		if (begins_with(c.keyword, command))
			return ParsedChatCommand {
				.command = c.command, 
				.text = payload 
			};
	}

	return ParsedChatCommand { ChatCommand::Invalid };
}