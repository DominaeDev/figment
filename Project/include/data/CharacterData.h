#ifndef CHARACTER_DATA_H__
#define CHARACTER_DATA_H__

#pragma once

#include "Figment.h"
#include "gui/GUITypes.h"
#include "data/CharacterGender.h"
#include "util/SearchIndex.h"

namespace fig::data
{
	struct CharacterProperty
	{
		fig::string label;
		fig::string value;

	public:
		static auto GetXmlFields();
	};

	struct CharacterData
	{
	public:
		fig::uuid assetId;
		fig::timestamp createdAt {};
		fig::timestamp updatedAt {};

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

		fig::io::FileError LoadFromXml(const fig::path& filename);
		fig::io::FileError LoadFromXml(const fig::string& doc);
		fig::io::FileError LoadFromXml(fig::string_view doc);
		void SaveToXml(fig::bytes& buffer) const;

	public:
		static auto GetXmlFields();
	};
}

#endif