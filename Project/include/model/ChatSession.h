#pragma once

#include "model/Character.h"

class ChatSession
{
public:

	Character user;
	std::vector<Character> characters {};
};