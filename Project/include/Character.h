#pragma once

#include "Types.h"

struct CharacterProperty 
{
	string key;
	string value;
	string label;
};

class Character
{
public:
	string name;
	string description;
	std::vector<CharacterProperty> properties;

	bool LoadFromXml(string filename);
};