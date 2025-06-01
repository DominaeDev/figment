#pragma once

#include "Types.h"
#include <vector>

struct llama_chat_message;

struct Message {
    string role;
    string content;
};

class ContextBuilder
{
public:
	string system_prompt;
	string persona;
	std::vector<Message> messages;


	std::vector<llama_chat_message> get_messages() const;
};
