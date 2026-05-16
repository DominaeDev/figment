#ifndef CHARACTER_DATA_H__
#define CHARACTER_DATA_H__

#pragma once

#include "Types.h"
#include "gui/GUITypes.h"
#include "util/SearchIndex.h"
#include "fs/XmlSerializable.h"

namespace fig::io
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

	struct CharacterData
	{
	public:
		fig::uuid assetId;
		fig::timestamp createdAt;
		fig::timestamp updatedAt;

		fig::string chatId;
		fig::string shortName;
		fig::string fullName;
		
		fig::string brief;
		fig::string description;
		fig::gui::Color bgColor {};
		fig::gui::Color borderColor {};
		CharacterGender gender {};

		string_list tags {};
		std::map<fig::string, CharacterProperty> properties;
		SearchIndex searchIndex;

		FileError LoadFromXml(const fig::path& filename);
		FileError LoadFromXml(const fig::string& doc);
		FileError LoadFromXml(fig::string_view doc);
		void SaveToXml(fig::bytes& buffer) const;

	public:
		static constexpr auto XmlFields()
		{
			return std::make_tuple(
				fig::io::XmlElement { &CharacterData::chatId, "ID" },
				fig::io::XmlElement { &CharacterData::shortName, "ShortName" },
				fig::io::XmlElement { &CharacterData::fullName, "FullName" },
				fig::io::XmlElement { &CharacterData::brief, "Brief" },
				fig::io::XmlElement { &CharacterData::description, "Description" },
				fig::io::XmlElement { &CharacterData::tags, "Tags" }
			);
		}
	};
}

#endif