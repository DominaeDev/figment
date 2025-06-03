#pragma once

#include "Command.h"

class CommandParser
{
public:
	static Command Parse(string text);
};