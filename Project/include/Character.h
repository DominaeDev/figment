#pragma once

#include "Types.h"

struct CharacterProperty {
	string key;
	string label;
	string value;
};

class Character
{
public:
	string name;
	string description;
	std::vector<CharacterProperty> properties;

	bool LoadFromXml(string filename);
};