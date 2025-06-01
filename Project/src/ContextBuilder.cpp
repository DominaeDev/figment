#include "ContextBuilder.h"
#include "StringUtil.h"
#include <llama.h>
#include <algorithm>
#include <string>
#include <ranges>

#define DEFAULT_USER "User"
#define DEFAULT_BOT "Assistant"

std::vector<llama_chat_message> ContextBuilder::GetMessages() const
{
	std::vector<Message> result;
	result.reserve(messages.size() + 8);

	if (!system_prompt.empty())
		result.push_back(Message { Role::System, system_prompt.c_str() });

	const Character& user = participants[0];
	const Character& bot = participants[1];
	string userName = user.name.empty() ? DEFAULT_USER : user.name;
	string botName = bot.name.empty() ? DEFAULT_BOT : bot.name;

	if (!bot.description.empty())
		result.push_back(Message { Role::System, bot.description.c_str() });

	result.insert(result.end(), std::begin(messages), std::end(messages));

	ReplacePlaceholders(result, "{{char}}", botName);
	ReplacePlaceholders(result, "{{user}}", userName);

	std::vector<llama_chat_message> llama_msgs;
	for (auto& m : result)
	{
		switch (m.role)
		{
		case Role::System:
			llama_msgs.push_back(llama_chat_message { _strdup("system"), _strdup(m.content.c_str()) });
			break;
		case Role::User:
			llama_msgs.push_back(llama_chat_message { _strdup(userName.c_str()), _strdup(m.content.c_str()) });
			break;
		case Role::Bot:
			llama_msgs.push_back(llama_chat_message { _strdup(botName.c_str()), _strdup(m.content.c_str()) });
			break;
		default:
			llama_msgs.push_back(llama_chat_message { _strdup(m.name.c_str()), _strdup(m.content.c_str()) });
		};
	}

	return llama_msgs;
}

bool ContextBuilder::LoadUser(string filename)
{
	return participants[0].LoadFromXml(filename);
}

bool ContextBuilder::LoadBot(string filename)
{
	return participants[1].LoadFromXml(filename);
}

void ContextBuilder::ReplacePlaceholders(std::vector<Message>& messages, string word, string replacement) const
{
	auto fnReplaceAll = [word, replacement](std::string& str)
	{
		auto&& pos = str.find(word);
		while (pos != std::string::npos)
		{
			str.replace(pos, word.length(), replacement);
			pos = str.find(word, pos + replacement.length());
		}
		return str;
	};

	for (auto& m : messages)
		fnReplaceAll(m.content);
}