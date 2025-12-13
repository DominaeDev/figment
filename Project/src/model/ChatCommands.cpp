#include "model/ChatCommands.h"
#include "util/StringUtility.h"

struct CommandDefinition {
	string keyword;
	ChatCommand command;
};

static CommandDefinition s_Commands[]
{
	{ "act",			ChatCommand::InstigateAction },
	{ "erase",			ChatCommand::RemoveLast },
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
	{ "talk",			ChatCommand::InstigateDialogue},
	{ "undo",			ChatCommand::RollbackUserMessage },
	{ "embed",			ChatCommand::GenerateEmbedding },
	{ "define",			ChatCommand::NewStateVariable },
	{ "set",			ChatCommand::SetStateVariable },
};

ParsedChatCommand ChatCommands::Parse(string text)
{
	string_util::trim_str(text);
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

	if (string_util::begins_with(text, "//")) // Shorthand
	{
		string payload = string_util::trim(text.substr(2));
		return ParsedChatCommand 
		{ 
			.command = ChatCommand::SystemMessage, 
			.text = payload,
		};
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
		if (string_util::begins_with(c.keyword, command))
			return ParsedChatCommand {
				.command = c.command, 
				.text = payload 
			};
	}

	return ParsedChatCommand { ChatCommand::Invalid };
}