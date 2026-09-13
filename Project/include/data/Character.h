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
		enum class ValueType
		{
			ShortText,
			LongText,
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

		fig::string name;
		fig::string value;
		ValueType format {};
		Visibility visibility {};
		HintFlags flags {};

	public:
		static auto XmlFields() noexcept;
	};

	struct CharacterName
	{
		fig::string first;
		fig::string last;
		fig::string nickname;

		static auto XmlFields() noexcept;

		fig::string GetSpokenName() const;
		fig::string GetFullName() const;
		bool empty() const;
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
		std::map<fig::string, CharacterAttribute>& GetAttributes() noexcept { return _attributes; }
		const std::map<fig::string, CharacterAttribute>& GetAttributes() const noexcept { return _attributes; }
		void SetAttribute(const fig::string& attributeId, const fig::string& label, fig::string_view content, CharacterAttribute::ValueType format = CharacterAttribute::ValueType::ShortText, CharacterAttribute::Visibility visibility = CharacterAttribute::Visibility::Public, CharacterAttribute::HintFlags flags = {});

		void SetTags(const fig::string_list& tags) noexcept;
		void AppendTags(const fig::string_list& tags);
		const fig::string_list& GetTags() const noexcept { return _tags; }

		void AddSearchTerm(const fig::string& term);
		const SearchIndex& GetSearchIndex() const noexcept { return _searchIndex; }

		[[nodiscard]] const Context& GetContext() noexcept;

		fig::string GetName() const noexcept { return name.GetSpokenName(); }
		fig::string GetFullName() const noexcept { return name.GetFullName(); }
		fig::string GetDescription() const noexcept;

	public:
		fig::string chatId;	//! @remove?
		CharacterName name;
		Gender gender {};
		Pronouns pronouns { Pronouns::Undefined };
		fig::string brief;
		
		fig::color bgColor {};
		fig::color borderColor {};

		// Meta data
		fig::string creator;
		fig::string about; // User-facing only
		fig::string version;
	
	private:
		void UpdateContext();

		std::map<fig::string, CharacterAttribute> _attributes;
		fig::string_list _tags {};
		SearchIndex _searchIndex;
		Context _context;
		bool _bDirtyContext {};

	public:
		static auto XmlFields() noexcept;
	};
}
