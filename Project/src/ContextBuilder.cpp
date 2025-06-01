#include "ContextBuilder.h"
#include <llama.h>

std::vector<llama_chat_message> ContextBuilder::get_messages() const
{
	std::vector<llama_chat_message> result;
	result.reserve(messages.size());
	for (auto& m : messages)
		result.push_back(llama_chat_message { m.role.c_str(), m.content.c_str() });
	return result;
}