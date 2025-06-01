#pragma once

#include "Types.h"
#include "Character.h"

struct llama_chat_message;

enum class Role
{
	System,
	User,
	Bot,
};

struct Message {
	Role role;
    string content;
    string name;
};

class ContextBuilder
{
public:

	std::vector<llama_chat_message> GetMessages() const;

	bool LoadUser(string filename);
	bool LoadBot(string filename);

public:
	std::vector<Character> participants { {}, {} };
	string system_prompt;
	std::vector<Message> messages;


private:
	void ReplacePlaceholders(std::vector<Message>& messages, string word, string replacement) const;
};
