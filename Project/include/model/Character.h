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
	string id;
	string name;
	string description;
	string portraitFilename;
	std::vector<CharacterProperty> properties;

	bool LoadFromXml(string filename);
};