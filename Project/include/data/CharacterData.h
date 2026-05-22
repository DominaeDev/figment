#ifndef CHARACTER_DATA_H__
#define CHARACTER_DATA_H__

#pragma once

#include "Figment.h"
#include "gui/GUITypes.h"
#include "data/CharacterGender.h"
#include "util/SearchIndex.h"

namespace fig::data
{
	struct CharacterAttribute
	{
		enum class Format
		{
			Text,
			Number,
			List,
		};

		enum class Visibility
		{
			Public,
			Private,
		};

		fig::string label;
		fig::string value;
		Format format {};
		Visibility visibility {};

	public:
		static auto GetXmlFields();
	};

	class CharacterData
	{
	public:
		fig::io::FileError LoadFromXml(const fig::path& filename);
		fig::io::FileError LoadFromXml(const fig::string& doc);
		fig::io::FileError LoadFromXml(fig::string_view doc);
		void SaveToXml(fig::bytes& buffer) const;

		std::optional<CharacterAttribute> FindAttribute(const fig::string_view& attributeId) const noexcept;
		std::optional<fig::string> GetAttribute(const fig::string_view& attributeId) const noexcept;
		inline const std::map<fig::string, CharacterAttribute>& GetAttributes() const noexcept { return _attributes; }

		inline const fig::string_list& GetTags() const noexcept { return _tags; }
		inline const SearchIndex& GetSearchIndex() const noexcept { return _searchIndex; }

		void AddAttribute(const fig::string& attributeId, const fig::string& label, const fig::string& content, CharacterAttribute::Format format = CharacterAttribute::Format::Text, CharacterAttribute::Visibility visibility = CharacterAttribute::Visibility::Public);
		void AppendTags(const fig::string_list& tags);
		void AddSearchTerm(const fig::string& term);

	public:
		fig::string chatId;	//! @remove?
		fig::string shortName;
		fig::string fullName;
		
		CharacterGender gender {};
		fig::string brief;
		fig::gui::Color bgColor {};
		fig::gui::Color borderColor {};
	
	private:
		std::map<fig::string, CharacterAttribute> _attributes;
		string_list _tags {};
		SearchIndex _searchIndex;

	public:
		static auto GetXmlFields();
	};

	using CharacterDataRef = std::reference_wrapper<fig::data::CharacterData>;
	using CharacterDataCRef = std::reference_wrapper<const fig::data::CharacterData>;
}

#endif