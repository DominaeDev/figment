#ifndef CHARACTER_H__
#define CHARACTER_H__

#pragma once

#include "Types.h"
#include "gui/GUITypes.h"

namespace fig::data
{
	struct CharacterProperty
	{
		fig::string key;
		fig::string value;
		fig::string label;
	};

	class Character
	{
	public:
		fig::string characterId;
		fig::string shortName;
		fig::string fullName;
		fig::string brief;
		fig::string description;
		fig::string portraitFilename;
		gui::Color bgColor {};
		gui::Color borderColor {};
		std::vector<CharacterProperty> properties;

		bool LoadFromXml(fig::string filename);
	};
}

#endif