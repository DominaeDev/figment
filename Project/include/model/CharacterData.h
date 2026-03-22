#ifndef CHARACTER_DATA_H__
#define CHARACTER_DATA_H__

#pragma once

#include "Types.h"
#include "gui/GUITypes.h"
#include "util/SearchIndex.h"

namespace fig::io::data
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
		fig::gui::Color bgColor {};
		fig::gui::Color borderColor {};
		CharacterGender gender {};

		fig::string largePortraitFilename;	//! @temp
		fig::string smallPortraitFilename;	//! @temp

		std::vector<fig::string> tags;
		std::map<fig::string, CharacterProperty> properties;
		SearchIndex searchIndex;

		bool LoadFromXml(const fig::path& filename);
		bool LoadFromXml(const fig::string& doc);
		bool LoadFromXml(fig::string_view doc);
		void SaveToXml(fig::bytes& buffer) const;
	};
}

#endif