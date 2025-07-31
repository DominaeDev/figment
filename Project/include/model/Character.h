#pragma once

#include "Types.h"
#include "gui/Graphics.h"

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
	string brief;
	string description;
	string portraitFilename;
	Color bgColor {};
	Color borderColor {};
	std::vector<CharacterProperty> properties;

	bool LoadFromXml(string filename);
};