#ifndef CHARACTER_H__
#define CHARACTER_H__

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
	string shortName;
	string fullName;
	string brief;
	string description;
	string portraitFilename;
	Color bgColor {};
	Color borderColor {};
	std::vector<CharacterProperty> properties;

	bool LoadFromXml(string filename);
};

#endif