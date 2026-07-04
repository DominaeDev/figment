#pragma once

#include "Figment.h"
#include "gui/GUITypes.h"
#include "data/CharacterGender.h"
#include "util/SearchIndex.h"
#include "text/Context.h"

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

		enum class HintFlag
		{
			Trivial		= 1 << 0,	// Can be omitted
			Important	= 1 << 1,	// Mustn't be omitted
			Memory		= 1 << 2,	// Can be placed in memory
		};
		using HintFlags = EnumFlags<HintFlag>;

		fig::string label;
		fig::string value;
		Format format {};
		Visibility visibility {};
		HintFlags flags {};

	public:
		static auto XmlFields() noexcept;
	};

	class Character
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

		[[nodiscard]] const Context& GetContext() noexcept;

	public:
		fig::string chatId;	//! @remove?
		fig::string shortName;
		fig::string fullName;
		fig::string brief;
		
		Gender gender {};
		fig::gui::Color bgColor {};
		fig::gui::Color borderColor {};
	
	private:
		void UpdateContext();

		std::map<fig::string, CharacterAttribute> _attributes;
		string_list _tags {};
		SearchIndex _searchIndex;
		Context _context;
		bool _bDirtyContext {};

	public:
		static auto XmlFields() noexcept;
	};
}
