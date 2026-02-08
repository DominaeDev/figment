#ifndef CHARACTER_H__
#define CHARACTER_H__

#pragma once

#include "Types.h"
#include "gui/GUITypes.h"

namespace fig::data
{
	struct CharacterProperty
	{
		fig::string label;
		fig::string value;
	};

	enum class CharacterGender
	{
		Undefined = 0,
		Male,
		Female,
		Custom,
	};

	class CharacterData
	{
	public:
		fig::string characterId; // Chat id
		fig::string shortName;
		fig::string fullName;
		fig::string subheader;
		fig::string brief;
		fig::string description;
		gui::Color bgColor {};
		gui::Color borderColor {};
		CharacterGender gender {};

		fig::string largePortraitFilename;
		fig::string smallPortraitFilename;

		std::map<fig::string, CharacterProperty> properties;

		bool LoadFromXml(const fig::path& filename);
		bool LoadFromXml(const fig::string& doc);
		void SaveToXml(fig::bytes& buffer);
	};
}

#endif